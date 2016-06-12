#pragma once

#include "DefaultAudio.hpp"
#include "FullModel.hpp"
#include "ImpulseRenderer.hpp"
#include "Ruler.hpp"
#include "Transport.hpp"

class ImpulseViewer : public Component,
                      public ApplicationCommandTarget,
                      public Button::Listener,
                      public Ruler::Listener,
                      public ScrollBar::Listener,
                      public Timer {
public:
    ImpulseViewer(AudioDeviceManager& audio_device_manager,
                  AudioFormatManager& audio_format_manager,
                  const File& file);
    virtual ~ImpulseViewer() noexcept;

    void resized() override;

    void getAllCommands(Array<CommandID>& commands) override;
    void getCommandInfo(CommandID command_id,
                        ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;
    ApplicationCommandTarget* getNextCommandTarget() override;

    void buttonClicked(Button* b) override;

    void timerCallback() override;

    void ruler_visible_range_changed(Ruler* r,
                                     const Range<double>& range) override;
    void scrollBarMoved(ScrollBar* s, double new_range_start) override;

private:
    AudioDeviceManager& audio_device_manager;

    AudioFormatReaderSource audio_format_reader_source;
    AudioTransportSource audio_transport_source;
    AudioSourcePlayer audio_source_player;

    ImpulseRendererComponent renderer;
    Ruler ruler;

    TextButton waterfall_button;
    TextButton waveform_button;

    ToggleButton follow_playback_button;

    ScrollBar scroll_bar;

    Transport transport;

    model::Connector<TextButton> waterfall_button_connector{&waterfall_button,
                                                            this};
    model::Connector<TextButton> waveform_button_connector{&waveform_button,
                                                           this};
    model::Connector<ToggleButton> follow_playback_connector{
            &follow_playback_button, this};
    model::Connector<Ruler> ruler_connector_0{&ruler, this};
    model::Connector<Ruler> ruler_connector_1{&ruler, &renderer.get_renderer()};
    model::Connector<ScrollBar> scroll_bar_connector{&scroll_bar, this};
};
