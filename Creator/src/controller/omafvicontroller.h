
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#pragma once

#include <string>
#include <functional>

#include "audio.h"
#include "common.h"
#include "dash.h"
#include "mp4vrwriter.h"

#include "controllerbase.h"

#include "async/asyncnode.h"
#include "async/graph.h"
#include "async/combinenode.h"
#include "common/utils.h"
#include "common/optional.h"
#include "concurrency/threadedpoolprocessor.h"
#include "config/config.h"
#include "dashomaf.h"

namespace VDD
{
    class Log;

    class MP4LoaderSource;

    class ControllerConfigure;

    class ControllerOps;

    class ConfigValueWithFallback;

    class OmafVIController : public ControllerBase
    {
    public:
        OmafVIController(Config aConfig);
        OmafVIController(const OmafVIController&) = delete;
        OmafVIController& operator=(const OmafVIController&) = delete;

        ~OmafVIController();
        void run();

        /** @brief moves errors to the client. Can only be called when 'run' is not being run. */
        GraphErrors moveErrors();

    protected:
        /** Builds the pipeline from a raw frame source (ie. import or tiling or audio) to saving
        segments. If there is no video encoder config, the video encoder is skipped and
        pipeline is built for raw data (ie AAC).

        @param aName Name of the output
        @param aDashOutput The Dash output configuration for this stream
        @param aPipelineOutput What kind of output is this? Mono, Left, Audio, etc
        @param aSources Which source to capture the initial frames from (raw import, aac import, or tiler)
        Uses aSegmenterOutput to choose the correct source from here.
        @param aMP4VRSink If this pipeline (the part before segmenter) is to be written to
        an MP4VR file, provide a writer sink

        @return Return the end of the pipeline, suitable for using with configureMPD
        */
        AsyncNode* buildPipeline(std::string aName,
            Optional<DashSegmenterConfig> aDashOutput,
            PipelineOutput aPipelineOutput,
            PipelineOutputNodeMap aSources,
            AsyncProcessor* aMP4VRSink);

        void makeAudioDashPipeline(const ConfigValue& aDashConfig,
            std::string aAudioName,
            AsyncNode* aAacInput,
            FrameDuration aTimeScale);

    private:
        std::shared_ptr<Log> mLog;
        std::shared_ptr<VDD::Config> mConfig;

        bool mParallel;
        std::unique_ptr<GraphBase> mGraph;
        ThreadedWorkPool mWorkPool;
        Optional<std::string> mDotFile;

        int mFrame = 1;

        GraphErrors mErrors;

        /* Graph ops lifting the shared functionality. unique_ptr so we don't need to include the
         * header. */
        std::unique_ptr<ControllerOps> mOps;

        DashOmaf mDash;

        SegmentDurations mDashDurations;

        /* Lower level configuration operations (ie. access to mInputLeft). */
        std::unique_ptr<ControllerConfigure> mConfigure;

        Optional<MP4VRWriter::Config> mMP4VRConfig;
        MP4VROutputs mMP4VROutputs;

        /** @brief Given an pipeline mode and base layer name, find an existing MP4VR producer for
         * it, or create one if it is missing and MP4VR generation is enabled. Arranges left and
         * right outputs of VideoSeparate to end up in the same MP4VRWriter.
         *
         * @return nullptr if mp4vr output is not enabled, otherwise a pointer to an MP4VRWriter */
        MP4VRWriter* createMP4VROutputForVideo(PipelineMode aMode, std::string aName);

        /** @brief Creates the video passthru pipeline. 
         */
        void makeVideo();

        /** @brief Reads keys in configuration under the "debug" and does some operations based on
         * them.
         *
         * Exporting output of a node: "debug": { "raw_export": { "5" : "foo" } }
         * or from command line: --debug.raw_export.3=debug
         **/
        void setupDebug();

        /** @brief Writes the dot file describing the graph, if enabled by { "debug" { "dot*:
         * "foo.dot" } } (maps to mDotFile) */
        void writeDot();

        friend class ControllerConfigure;
    };
}
