#pragma once

#include <QDialog>

class QComboBox;
class QLineEdit;
class QRadioButton;

class AppData;
class TableController;

// Unified import dialog: choose a JSON file, target table, an optional mod to
// tag the imported entries with, merge/replace mode, and a "import tutorial"
// button that shows a simple format example.
class ImportDialog : public QDialog
{
    Q_OBJECT
public:
    ImportDialog(AppData* data, TableController* tables,
                 const QString& preselectMod = QString(),
                 QWidget* parent = nullptr);

private slots:
    void browse();
    void showTutorial();
    void doImport();

private:
    void refreshModCombo(const QString& selectMod);

    // Sentinel item data values for the mod combo.
    static const char* kKeepFromFile;
    static const char* kVanilla;

    AppData* m_data = nullptr;
    TableController* m_tables = nullptr;
    QLineEdit* m_path = nullptr;
    QComboBox* m_tableCombo = nullptr;
    QComboBox* m_modCombo = nullptr;
    QRadioButton* m_merge = nullptr;
    QRadioButton* m_replace = nullptr;
};
