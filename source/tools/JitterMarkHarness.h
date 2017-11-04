/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <iomanip>
#include <math.h>
#include "AudioSinkBase.h"
#include "SynthMark.h"
#include "synth/Synthesizer.h"
#include "tools/CpuAnalyzer.h"
#include "tools/TimingAnalyzer.h"
#include "tools/LogTool.h"
#include "tools/TestHarnessBase.h"

#ifndef SYNTHMARK_JITTERMARK_HARNESS_H
#define SYNTHMARK_JITTERMARK_HARNESS_H

#define JITTER_BINS_PER_MSEC  10
#define JITTER_MAX_MSEC       100

/**
 * Determine buffer latency required to avoid glitches.
 * The "LatencyMark" is the minimum buffer size that is a multiple
 * of a block size that can be used for N minutes without glitching.
 */
class JitterMarkHarness : public TestHarnessBase {
public:
    JitterMarkHarness(AudioSinkBase *audioSink, SynthMarkResult *result, LogTool *logTool = NULL)
    : TestHarnessBase(audioSink, result, logTool)
    , mNanosPerBin(1)
    {
        mTestName = "JitterMark";
    }

    virtual ~JitterMarkHarness() {
    }

    virtual void onBeginMeasurement() override {
        mResult->setTestName(mTestName);
        mLogTool->log("---- Measure scheduling jitter ---- #voices = %d\n", mNumVoices);
        /*
        int32_t burstsPerBuffer = 8;
        int32_t desiredSizeInFrames = burstsPerBuffer * mFramesPerBurst;
        int32_t actualSizeInFrames = mAudioSink->setBufferSizeInFrames(desiredSizeInFrames);
        if (actualSizeInFrames < desiredSizeInFrames) {
            mLogTool->log("WARNING - could not set desired buffer size\n");
        }
         */
        // set resolution and size of histogram
        int32_t nanosPerMilli = (int32_t)(SYNTHMARK_NANOS_PER_SECOND / SYNTHMARK_MILLIS_PER_SECOND);
        mNanosPerBin = nanosPerMilli / JITTER_BINS_PER_MSEC;
        int32_t numBins = JITTER_MAX_MSEC * JITTER_BINS_PER_MSEC;
        mTimer.setupJitterRecording(mNanosPerBin, numBins);
    }

    virtual int32_t onBeforeNoteOn() override {
        return 0;
    }

    virtual void onEndMeasurement() override {

        double measurement = 0.0;
        std::stringstream resultMessage;
        resultMessage << mTestName << " = " << measurement << std::endl;

        // Print jitter histogram
        BinCounter *wakeupBins = mTimer.getWakeupBins();
        BinCounter *renderBins = mTimer.getRenderBins();
        BinCounter *deliveryBins = mTimer.getDeliveryBins();
        if (wakeupBins != NULL && renderBins != NULL  && deliveryBins != NULL ) {
            int32_t numBins = deliveryBins->getNumBins();
            const int32_t *wakeupCounts = wakeupBins->getBins();
            const int32_t *wakeupLast = wakeupBins->getLastMarkers();
            const int32_t *renderCounts = renderBins->getBins();
            const int32_t *renderLast = renderBins->getLastMarkers();
            const int32_t *deliveryCounts = deliveryBins->getBins();
            const int32_t *deliveryLast = deliveryBins->getLastMarkers();
            resultMessage << " bin#,  msec,"
                    << "   wakeup#,  wlast,"
                    << "   render#,  rlast,"
                    << " delivery#,  clast" << std::endl;
            for (int i = 0; i < numBins; i++) {
                if (wakeupCounts[i] > 0 || renderCounts[i] > 0 || deliveryCounts[i] > 0) {
                    double msec = (double) i * mNanosPerBin * SYNTHMARK_MILLIS_PER_SECOND
                                  / SYNTHMARK_NANOS_PER_SECOND;
                    resultMessage << "  " << std::setw(3) << i
                    << ", " << std::fixed << std::setw(5) << std::setprecision(2) << msec
                    << ", " << std::setw(9) << wakeupCounts[i]
                    << ", " << std::setw(6) << wakeupLast[i]
                    << ", " << std::setw(9) << renderCounts[i]
                    << ", " << std::setw(6) << renderLast[i]
                    << ", " << std::setw(9) << deliveryCounts[i]
                    << ", " << std::setw(6) << deliveryLast[i]
                    << std::endl;
                }
            }
        } else {
            resultMessage << "ERROR NULL BinCounter!\n";
        }
        resultMessage << "Underruns " << mAudioSink->getUnderrunCount() << "\n";

        resultMessage << mCpuAnalyzer.dump();

        mResult->setMeasurement(measurement);

        mResult->setResultMessage(resultMessage.str());
    }


    int32_t mNanosPerBin;

};

#endif // SYNTHMARK_JITTERMARK_HARNESS_H
