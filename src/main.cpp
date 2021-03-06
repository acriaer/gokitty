#include <stdio.h>

#include "config.h"
#include "data_reader.h"
#include "executor.h"
#include "hinge_model.h"
#include "integration.h"
#include "log.h"
#include "visualisation.h"

using std::string;

#define STAGE_RECORD 0
#define STAGE_RACE 1

int main(int argc, char **argv)
{
    Log log("main");
    log.Info() << "gokitty demo application";

    Config::inst().Load(argc, argv);

    auto config_path = Config::inst().GetOption<std::string>("config");
    if (config_path != "")
    {
        Config::inst().Load(config_path);
    }

    LoggingSingleton::inst().SetConsoleVerbosity(
        Config::inst().GetOption<bool>("verbose"));
    LoggingSingleton::inst().AddLogFile(
        Config::inst().GetOption<std::string>("log_file"));

    Config::inst().DumpSettings();

    //====================

    adept::Stack main_stack;

    std::unique_ptr<Visualisation> vis;
    if (Config::inst().GetOption<bool>("gui"))
        vis = std::make_unique<Visualisation>();

    if (Config::inst().GetOption<int>("stage") == STAGE_RACE)
    {
        int model_size_x = Config::inst().GetOption<int>("board_width");
        int model_size_y = Config::inst().GetOption<int>("board_height");
        double model_cell = Config::inst().GetOption<float>("board_cell");
        double score = FLT_MAX;

        auto track_start = Vector<2, false>({{double(model_size_x) * model_cell / 2.0,
                                              double(model_size_y) * model_cell / 2.0}});

        HingeModel model(model_size_x, model_size_y, model_cell);
        DataReader::ReadTORCSTrack(Config::inst().GetOption<string>("track"), model,
                                   track_start);

        if (Config::inst().GetOption<string>("model_path") != "")
        {
            DataReader::ReadHingeModel(Config::inst().GetOption<string>("model_path"),
                                       model);
            adept::aReal ascore = 0.0;
            model.SetupEquations();
            model.ComputeScore(ascore);
            score = ascore.value();
            log.Info() << "Loaded hinge model. Its score is " << score << ".";
        }

        std::unique_ptr<TorcsIntegration> integration;
        ExecutorRacing executor(model);

        if (vis)
            vis->SetCameraPos(track_start);

        bool optimization_paused = true;
        bool exit_requested = false;
        CarState car_state;

        while (!exit_requested)
        {
            if (!optimization_paused)
            {
                for (int i = 0;
                     i < Config::inst().GetOption<int>("optimizations_per_frame"); i++)
                {
                    score = model.Optimize(main_stack);
                }
            }
            else
            {
                main_stack.new_recording();
            }

            if (!integration &&
                score <= Config::inst().GetOption<float>("score_threshold"))
            {
                auto track_name = Config::inst().GetOption<string>("track");
                std::replace(track_name.begin(), track_name.end(), '/', '_');

                DataReader::SaveHingeModel(
                    Config::inst().GetOption<string>("save_model_path_prefix") +
                        track_name + "_" + std::to_string(score) + ".hinges",
                    model);

                integration = std::make_unique<TorcsIntegration>();
                car_state = integration->Begin();
            }

            if (integration)
            {
                auto steers = executor.Cycle(car_state, 1.0);
                car_state = integration->Cycle(steers);
            }

            if (vis)
            {
                std::vector<Visualisation::Object> objects;

                model.Visualise(objects);
                executor.Visualise(objects);
                vis->Tick(objects);

                while (auto action = vis->DequeueAction())
                {
                    switch (*action)
                    {
                    case Visualisation::Exit:
                        exit_requested = true;
                        break;
                    case Visualisation::OptimizationPause:
                        optimization_paused = !optimization_paused;
                        break;
                    default:
                        ASSERT(0, "Action not implemented!")
                    }
                }
            }
        }
    }

    if (Config::inst().GetOption<int>("stage") == STAGE_RECORD)
    {
        TorcsIntegration integration;
        ExecutorRecording executor;

        auto state = integration.Begin();
        while (!executor.RecordingDone())
        {
            auto steers = executor.Cycle(state, 1.0);
            state = integration.Cycle(steers);

            if (vis)
            {
                std::vector<Visualisation::Object> objects;
                executor.Visualise(objects);
                vis->Tick(objects);
            }
        }
    }

    log.Info() << "Nothing more to do.";
}