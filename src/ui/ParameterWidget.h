#pragma once

#include <QWidget>
#include <functional>

class QComboBox;
class QFormLayout;
class QLabel;
class QStackedWidget;
class QVBoxLayout;

class AppData;
class CommandController;
class Command;
struct Parameter;

// Dynamically builds a form for the currently selected command. One row per
// parameter; the widget type depends on the parameter type, and multi-type
// parameters get a mode selector with a stacked widget.
class ParameterWidget : public QWidget
{
    Q_OBJECT
public:
    ParameterWidget(AppData* data, CommandController* controller, QWidget* parent = nullptr);

    // True when every required parameter currently has a non-empty value.
    bool isComplete() const;

private slots:
    void rebuild();

private:
    struct Editor {
        QWidget* widget = nullptr;
        std::function<QString()> token;   // current token ("" = empty)
        std::function<bool()> filled;     // whether it participates in output
    };

    // Builds the editor for a single parameter, connecting signals to the
    // controller. Returns the editor bundle.
    Editor createEditor(const Parameter& param, int idx);
    Editor createLookupEditor(const Parameter& param, const QString& tableType, int idx);
    Editor createLocationEditor(const Parameter& param, int idx);
    Editor createMapEditor(const Parameter& param, int idx);
    Editor createEnumEditor(const Parameter& param, int idx);
    Editor createBooleanEditor(const Parameter& param, int idx);
    Editor createIntegerEditor(const Parameter& param, bool isFloat, int idx);
    Editor createTextEditor(const Parameter& param, const QString& placeholder, int idx);
    Editor createColorEditor(const Parameter& param, int idx);
    Editor createDurationEditor(const Parameter& param, int idx);
    Editor createMultiTypeEditor(const Parameter& param, int idx);

    void addEditorRow(const Parameter& param, int idx, const Editor& ed);
    void refreshWarning();

    AppData* m_data = nullptr;
    CommandController* m_controller = nullptr;
    QFormLayout* m_form = nullptr;
    QLabel* m_warning = nullptr;
    const Command* m_cmd = nullptr;
};
