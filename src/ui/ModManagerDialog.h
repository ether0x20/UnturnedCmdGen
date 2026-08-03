#pragma once

#include <QDialog>

class QTableWidget;

class AppData;
class TableController;

// Mod resource manager: lists every known mod (plus the implicit vanilla mod),
// toggles enable/disable (which filters entries out of the selection UI),
// and lets the user add/remove mods, import data into a mod, or open a
// per-mod table manager.
class ModManagerDialog : public QDialog
{
    Q_OBJECT
public:
    ModManagerDialog(AppData* data, TableController* tables, QWidget* parent = nullptr);

private slots:
    void addMod();
    void removeSelectedMod();
    void importModData();
    void manageEntries();
    void refresh();

private:
    QString selectedModId() const;
    void applyModToggle(const QString& modId, bool enabled);

    AppData* m_data = nullptr;
    TableController* m_tables = nullptr;
    QTableWidget* m_table = nullptr;
};
