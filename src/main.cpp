#include <stdio.h>

#include "config.h"
#include "data_reader.h"
#include "hinge_model.h"
#include "log.h"
#include "visualisation.h"

using std::string;

int main(int argc, char **argv)
{
    Log log("main");
    log.Info() << "gokitty demo application";
    adept::Stack main_stack;

    Config::inst().Load(argc, argv);

    auto config_path = Config::inst().GetOption<std::string>("config");
    if (config_path != "")
    {
        Config::inst().Load(config_path);
        Config::inst().Load(argc, argv); // override params from config file
    }

    LoggingSingleton::inst().SetConsoleVerbosity(
        Config::inst().GetOption<bool>("verbose"));
    LoggingSingleton::inst().AddLogFile(
        Config::inst().GetOption<std::string>("log_file"));

    Config::inst().DumpSettings();

    //====================

    HingeModel model(100, 100, 10.0f);
    Visualisation vis;
    DataReader::ReadTORCSTrack(Config::inst().GetOption<string>("track"), model);

    while (!vis.ExitRequested())
    {
        std::vector<Visualisation::Object> objects;
        model.Visualise(objects);
        vis.Tick(objects);
        model.Optimize(main_stack);
    }
}