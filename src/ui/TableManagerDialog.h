#pragma once

#include <QDialog>

class QTableWidget;

class AppData;
class TableController;

// Generic CRUD manager for any lookup table (item/vehicle/animal/effect/quest/
// achievement/skillset/player). Column headers adapt to the table type, and
// changes are persisted back to the table's source file on save.
class TableManagerDialog : public QDialog
{
    Q_OBJECT
public:
    TableManagerDialog(AppData* data, TableController* tables,
                       const QString& tableType, QWidget* parent = nullptr);

    // Localized plural display name for a table key (e.g. "item" -> "Items").
    static QString displayName(const QString& tableType);

private slots:
    void addRow();
    void removeSelected();
    void save();

private:
    AppData* m_data = nullptr;
    TableController* m_tables = nullptr;
    QString m_tableType;
    QTableWidget* m_table = nullptr;
};
