#pragma once

#include <QDialog>

class QComboBox;
class QLineEdit;
class QRadioButton;

class AppData;
class TableController;

// Unified import dialog: choose a JSON file, target table, merge/replace mode,
// and a "import tutorial" button that shows a simple format example.
class ImportDialog : public QDialog
{
    Q_OBJECT
public:
    ImportDialog(AppData* data, TableController* tables, QWidget* parent = nullptr);

private slots:
    void browse();
    void showTutorial();
    void doImport();

private:
    AppData* m_data = nullptr;
    TableController* m_tables = nullptr;
    QLineEdit* m_path = nullptr;
    QComboBox* m_tableCombo = nullptr;
    QRadioButton* m_merge = nullptr;
    QRadioButton* m_replace = nullptr;
};
