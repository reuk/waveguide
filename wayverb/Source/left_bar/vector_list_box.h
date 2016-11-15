#pragma once

#include "combined/model/vector.h"

#include "../JuceLibraryCode/JuceHeader.h"

#include <functional>

namespace left_bar {

/// Dead simple list box editor.
/// Listens to a model::vector, and updates if the model changes.
/// Allows the component for each row to be created using a factory callback.
template <typename Model>
class vector_list_box final : public ListBox {
public:
    using model_type = Model;

    using new_component_for_row =
            std::function<std::unique_ptr<Component>(int row, bool selected)>;

    vector_list_box(Model& model, new_component_for_row new_component_for_row)
            : model_{model, std::move(new_component_for_row)} {
        /// If model changes, update the listbox view.
        model.connect([this](auto&) { this->updateContent(); });

        setModel(&model_);
    }

private:
    class model final : public ListBoxModel {
    public:
        model(Model& model, new_component_for_row new_component_for_row)
                : model_{model}
                , new_component_for_row_{std::move(new_component_for_row)} {}

        int getNumRows() override { return model_.size(); }

        void paintListBoxItem(int, Graphics&, int, int, bool) override {}

        Component* refreshComponentForRow(int row,
                                          bool selected,
                                          Component* existing) override {
            if (existing) {
                delete existing;
                existing = nullptr;
            }
            if (row < getNumRows()) {
                existing = new_component_for_row_(row, selected).release();
            }
            return existing;
        }

    private:
        Model& model_;
        new_component_for_row new_component_for_row_;
    };

    model model_;
};

}  // namespace left_bar
