///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2017 Edouard Griffiths, F4EXB                                   //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINS_CHANNELTX_MODATV_ATVMOD_H_
#define PLUGINS_CHANNELTX_MODATV_ATVMOD_H_

#include <QObject>
#include <QMutex>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdint.h>

#include "dsp/basebandsamplesource.h"
#include "dsp/nco.h"
#include "dsp/interpolator.h"
#include "dsp/movingaverage.h"
#include "util/message.h"

class ATVMod : public BasebandSampleSource {
    Q_OBJECT

public:
    typedef enum
    {
        ATVStdPAL625,
		ATVStdPAL525
    } ATVStd;

    typedef enum
    {
        ATVModInputUniform,
        ATVModInputHBars,
        ATVModInputVBars,
        ATVModInputChessboard,
        ATVModInputHGradient,
        ATVModInputVGradient,
        ATVModInputImage,
        ATVModInputVideo
    } ATVModInput;

    typedef enum
    {
    	ATVModulationAM,
		ATVModulationFM
    } ATVModulation;

    class MsgConfigureImageFileName : public Message
    {
        MESSAGE_CLASS_DECLARATION

    public:
        const QString& getFileName() const { return m_fileName; }

        static MsgConfigureImageFileName* create(const QString& fileName)
        {
            return new MsgConfigureImageFileName(fileName);
        }

    private:
        QString m_fileName;

        MsgConfigureImageFileName(const QString& fileName) :
            Message(),
            m_fileName(fileName)
        { }
    };

    ATVMod();
    ~ATVMod();

    void configure(MessageQueue* messageQueue,
            Real rfBandwidth,
            ATVStd atvStd,
            ATVModInput atvModInput,
            Real uniformLevel,
			ATVModulation atvModulation,
            bool channelMute);

    virtual void pull(Sample& sample);
    virtual void pullAudio(int nbSamples); // this is used for video signal actually
    virtual void start();
    virtual void stop();
    virtual bool handleMessage(const Message& cmd);

    Real getMagSq() const { return m_movingAverage.average(); }

    static int getSampleRateUnits(ATVStd std);

signals:
    /**
     * Level changed
     * \param rmsLevel RMS level in range 0.0 - 1.0
     * \param peakLevel Peak level in range 0.0 - 1.0
     * \param numSamples Number of audio samples analyzed
     */
    void levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples);

private:
    class MsgConfigureATVMod : public Message
    {
        MESSAGE_CLASS_DECLARATION

    public:
        Real getRFBandwidth() const { return m_rfBandwidth; }
        ATVStd getATVStd() const { return m_atvStd; }
        ATVModInput getATVModInput() const { return m_atvModInput; }
        Real getUniformLevel() const { return m_uniformLevel; }
        ATVModulation getModulation() const { return m_atvModulation; }

        static MsgConfigureATVMod* create(
            Real rfBandwidth,
            ATVStd atvStd,
            ATVModInput atvModInput,
            Real uniformLevel,
			ATVModulation atvModulation)
        {
            return new MsgConfigureATVMod(rfBandwidth, atvStd, atvModInput, uniformLevel, atvModulation);
        }

    private:
        Real          m_rfBandwidth;
        ATVStd        m_atvStd;
        ATVModInput   m_atvModInput;
        Real          m_uniformLevel;
        ATVModulation m_atvModulation;

        MsgConfigureATVMod(
                Real rfBandwidth,
                ATVStd atvStd,
                ATVModInput atvModInput,
                Real uniformLevel,
				ATVModulation atvModulation) :
            Message(),
            m_rfBandwidth(rfBandwidth),
            m_atvStd(atvStd),
            m_atvModInput(atvModInput),
            m_uniformLevel(uniformLevel),
			m_atvModulation(atvModulation)
        { }
    };

    struct Config
    {
        int           m_outputSampleRate;     //!< sample rate from channelizer
        qint64        m_inputFrequencyOffset; //!< offset from baseband center frequency
        Real          m_rfBandwidth;          //!< Bandwidth of modulated signal
        ATVStd        m_atvStd;               //!< Standard
        ATVModInput   m_atvModInput;          //!< Input source type
        Real          m_uniformLevel;         //!< Percentage between black and white for uniform screen display
        ATVModulation m_atvModulation;        //!< RF modulation type

        Config() :
            m_outputSampleRate(-1),
            m_inputFrequencyOffset(0),
            m_rfBandwidth(0),
            m_atvStd(ATVStdPAL625),
            m_atvModInput(ATVModInputHBars),
            m_uniformLevel(0.5f),
			m_atvModulation(ATVModulationAM)
        { }
    };

    Config m_config;
    Config m_running;

    NCO m_carrierNco;
    Complex m_modSample;
    float m_modPhasor; //!< For FM modulation
    Interpolator m_interpolator;
    Real m_interpolatorDistance;
    Real m_interpolatorDistanceRemain;
    int      m_tvSampleRate;     //!< sample rate for generating signal
    uint32_t m_pointsPerSync;    //!< number of line points for the horizontal sync
    uint32_t m_pointsPerBP;      //!< number of line points for the back porch
    uint32_t m_pointsPerImgLine; //!< number of line points for the image line
    uint32_t m_pointsPerFP;      //!< number of line points for the front porch
    uint32_t m_pointsPerFSync;   //!< number of line points for the field first sync
    uint32_t m_pointsPerHBar;    //!< number of line points for a bar of the bar chart
    uint32_t m_linesPerVBar;     //!< number of lines for a bar of the bar chart
    uint32_t m_pointsPerTU;      //!< number of line points per time unit
    uint32_t m_nbLines;          //!< number of lines per complete frame
    uint32_t m_nbLines2;         //!< same number as above (non interlaced) or half the number above (interlaced)
    uint32_t m_nbImageLines;     //!< number of image lines excluding synchronization lines
    uint32_t m_nbImageLines2;    //!< same number as above (non interlaced) or half the number above (interlaced)
    uint32_t m_nbHorizPoints;    //!< number of line points per horizontal line
    uint32_t m_nbSyncLinesH;     //!< number of header sync lines
    uint32_t m_nbBlankLines;     //!< number of lines in a frame (full or half) that are blanked (black) at the top of the image
    float    m_hBarIncrement;    //!< video level increment at each horizontal bar increment
    float    m_vBarIncrement;    //!< video level increment at each vertical bar increment
    bool     m_interlaced;       //!< true if image is interlaced (2 half frames per frame)
    bool     m_evenImage;        //!< in interlaced mode true if this is an even image
    QMutex   m_settingsMutex;
    int      m_horizontalCount;  //!< current point index on line
    int      m_lineCount;        //!< current line index in frame

    MovingAverage<Real> m_movingAverage;
    quint32 m_levelCalcCount;
    Real m_peakLevel;
    Real m_levelSum;

    cv::Mat m_imageOriginal;     //!< original non resized image
    cv::Mat m_image;             //!< resized image for transmission at given rate
    bool m_imageOK;

    static const float m_blackLevel;
    static const float m_spanLevel;
    static const int m_levelNbSamples;
    static const int m_nbBars; // number of bars in bar or chessboard patterns

    void apply(bool force = false);
    void pullFinalize(Complex& ci, Sample& sample);
    void pullVideo(Real& sample);
    void calculateLevel(Real& sample);
    void modulateSample();
    void applyStandard();
    void openImage(const QString& fileName);
    void resizeImage();

    inline void pullImageLine(Real& sample)
    {
        if (m_horizontalCount < m_pointsPerSync) // sync pulse
        {
            sample = 0.0f; // ultra-black
        }
        else if (m_horizontalCount < m_pointsPerSync + m_pointsPerBP) // back porch
        {
            sample = m_blackLevel; // black
        }
        else if (m_horizontalCount < m_pointsPerSync + m_pointsPerBP + m_pointsPerImgLine)
        {
            int pointIndex = m_horizontalCount - (m_pointsPerSync + m_pointsPerBP);
            int iLine = m_lineCount % m_nbLines2;
            int oddity = m_lineCount < m_nbLines2 ? 0 : 1;
            int iLineImage = iLine - m_nbSyncLinesH - m_nbBlankLines;

            switch(m_running.m_atvModInput)
            {
            case ATVModInputHBars:
                sample = (pointIndex / m_pointsPerHBar) * m_hBarIncrement + m_blackLevel;
                break;
            case ATVModInputVBars:
                sample = (iLine / m_linesPerVBar) * m_vBarIncrement + m_blackLevel;
                break;
            case ATVModInputChessboard:
                sample = (((iLine / m_linesPerVBar)*5 + (pointIndex / m_pointsPerHBar)) % 2) * m_spanLevel * m_running.m_uniformLevel + m_blackLevel;
                break;
            case ATVModInputHGradient:
                sample = (pointIndex / (float) m_pointsPerImgLine) * m_spanLevel + m_blackLevel;
                break;
            case ATVModInputVGradient:
                sample = ((iLine -5) / (float) m_nbImageLines2) * m_spanLevel + m_blackLevel;
                break;
            case ATVModInputImage:
                if (!m_imageOK || (iLineImage < 0))
                {
                    sample = m_spanLevel * m_running.m_uniformLevel + m_blackLevel;
                }
                else
                {
                    unsigned char pixv = m_image.at<unsigned char>(2*iLineImage+ oddity, pointIndex); // row (y), col (x)
                    sample = (pixv / 256.0f) * m_spanLevel + m_blackLevel;
                }
                break;
            case ATVModInputUniform:
            default:
                sample = m_spanLevel * m_running.m_uniformLevel + m_blackLevel;
            }
        }
        else // front porch
        {
            sample = m_blackLevel; // black
        }
    }

    inline void pullVSyncLine(Real& sample)
    {
        int fieldLine = m_lineCount % m_nbLines2;

        if (m_lineCount < m_nbLines2) // even
        {
            if (fieldLine < 2) // 0,1: Whole line "long" pulses
            {
                int halfIndex = m_horizontalCount % (m_nbHorizPoints/2);

                if (halfIndex < (m_nbHorizPoints/2) - m_pointsPerSync) // ultra-black
                {
                    sample = 0.0f;
                }
                else // black
                {
                    sample = m_blackLevel;
                }
            }
            else if (fieldLine == 2) // long pulse then equalizing pulse
            {
                if (m_horizontalCount < (m_nbHorizPoints/2) - m_pointsPerSync)
                {
                    sample = 0.0f; // ultra-black
                }
                else if (m_horizontalCount < (m_nbHorizPoints/2))
                {
                    sample = m_blackLevel; // black
                }
                else if (m_horizontalCount < (m_nbHorizPoints/2) + m_pointsPerFSync)
                {
                    sample = 0.0f; // ultra-black
                }
                else
                {
                    sample = m_blackLevel; // black
                }
            }
            else if ((fieldLine < 5) || (fieldLine > m_nbLines2 - 3)) // Whole line equalizing pulses
            {
                int halfIndex = m_horizontalCount % (m_nbHorizPoints/2);

                if (halfIndex < m_pointsPerFSync) // ultra-black
                {
                    sample = 0.0f;
                }
                else // black
                {
                    sample = m_blackLevel;
                }
            }
            else // black images
            {
                if (m_horizontalCount < m_pointsPerSync)
                {
                    sample = 0.0f;
                }
                else
                {
                    sample = m_blackLevel;
                }
            }
        }
        else // odd
        {
            if (fieldLine < 1) // equalizing pulse then long pulse
            {
                if (m_horizontalCount < m_pointsPerFSync)
                {
                    sample = 0.0f; // ultra-black
                }
                else if (m_horizontalCount < (m_nbHorizPoints/2))
                {
                    sample = m_blackLevel; // black
                }
                else if (m_horizontalCount < m_nbHorizPoints - m_pointsPerSync)
                {
                    sample = 0.0f; // ultra-black
                }
                else
                {
                    sample = m_blackLevel; // black
                }
            }
            else if (fieldLine < 3) // Whole line "long" pulses
            {
                int halfIndex = m_horizontalCount % (m_nbHorizPoints/2);

                if (halfIndex < (m_nbHorizPoints/2) - m_pointsPerSync) // ultra-black
                {
                    sample = 0.0f;
                }
                else // black
                {
                    sample = m_blackLevel;
                }
            }
            else if ((fieldLine < 5) || (fieldLine > m_nbLines2 - 5)) // Whole line equalizing pulses
            {
                int halfIndex = m_horizontalCount % (m_nbHorizPoints/2);

                if (halfIndex < m_pointsPerFSync) // ultra-black
                {
                    sample = 0.0f;
                }
                else // black
                {
                    sample = m_blackLevel;
                }
            }
            else // black images
            {
                if (m_horizontalCount < m_pointsPerSync)
                {
                    sample = 0.0f;
                }
                else
                {
                    sample = m_blackLevel;
                }
            }
        }
    }
};


#endif /* PLUGINS_CHANNELTX_MODATV_ATVMOD_H_ */