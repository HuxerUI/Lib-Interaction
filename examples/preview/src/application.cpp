#include "app.h"

huxerui::View App() { return interaction_demo::PreviewApp(); }

const huxerui::Application application{
    App,
    {.window = {.title = "Lib-Interaction Preview", .initial_size = {1100.0F, 840.0F}}},
};
