#pragma once

#include <QWidget>

class QCheckBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

class AppData;
class CommandController;

// Left panel: search box + classification filter checkboxes + command list.
// Emits commandSelected() when the user picks a command.
class CommandListWidget : public QWidget
{
    Q_OBJECT
public:
    CommandListWidget(AppData* data, CommandController* controller, QWidget* parent = nullptr);

    void selectFirst();
    QString currentCommand() const;
    void applyFilter();

signals:
    void commandSelected(const QString& name);

private slots:
    void onItemActivated(QListWidgetItem* item);

private:
    AppData* m_data = nullptr;
    CommandController* m_controller = nullptr;
    QLineEdit* m_search = nullptr;
    QCheckBox* m_runtime = nullptr;
    QCheckBox* m_cheat = nullptr;
    QCheckBox* m_config = nullptr;
    QCheckBox* m_any = nullptr;
    QListWidget* m_list = nullptr;
};
