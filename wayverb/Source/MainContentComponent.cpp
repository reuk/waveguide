#include "MainContentComponent.h"

#include "core/serialize/surface.h"

#include <iomanip>
#include <type_traits>

MainContentComponent::MainContentComponent(main_model& model)
        : left_bar_master_{model}
        , resizer_bar_{&layout_manager_, 1, true}
        , scene_master_{model}
        , components_to_position_{
                  &left_bar_master_, &resizer_bar_, &scene_master_} {
    const auto left_bar_master_width = 300;
    layout_manager_.setItemLayout(0,
                                  left_bar_master_width,
                                  left_bar_master_width,
                                  left_bar_master_width);
    const auto bar_width = 0;
    layout_manager_.setItemLayout(1, bar_width, bar_width, bar_width);
    layout_manager_.setItemLayout(2, 300, 10000, 400);

    addAndMakeVisible(left_bar_master_);
    addAndMakeVisible(resizer_bar_);
    addAndMakeVisible(scene_master_);
}

void MainContentComponent::paint(Graphics& g) { g.fillAll(Colours::darkgrey); }

void MainContentComponent::resized() {
    layout_manager_.layOutComponents(components_to_position_,
                                     num_components_,
                                     0,
                                     0,
                                     getWidth(),
                                     getHeight(),
                                     false,
                                     true);
}

constexpr size_t MainContentComponent::num_components_;
