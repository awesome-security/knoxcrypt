#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "WorkThread.h"
#include "LoaderThread.h"
#include "utility/EventType.hpp"
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <QMainWindow>
#include <QProgressDialog>
#include <QMenu>
#include <QAction>
#include <set>

namespace Ui
{
    class MainWindow;
}

namespace teasafe
{
    class TeaSafe;
}

typedef boost::shared_ptr<teasafe::TeaSafe> SharedTeaSafe;

enum class WorkType { RemoveItem, CreateFolder, ExtractItem };

class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = 0);

    ~MainWindow();

    void cipherCallback(teasafe::EventType eventType, long const amount);

  public slots:
    void finishedLoadingSlot();
    /**
     * @brief loadFileButtonHandler for loading a TeaSafe image
     */
    void loadFileButtonHandler();

    void updateProgressSlot();
    void cipherGeneratedSlot();
    void setMaximumProgressSlot(long value);
    void extractClickedSlot();
    void removedClickedSlot();
    void newFolderClickedSlot();

    void itemExpanded(QTreeWidgetItem *);

  signals:
    void updateProgressSignal();
    void cipherGeneratedSIgnal();
    void setMaximumProgressSignal(long);

  private:
    Ui::MainWindow *ui;
    SharedTeaSafe m_teaSafe;
    LoaderThread m_loaderThread;
    WorkThread m_workThread;
    boost::shared_ptr<QMenu> m_contextMenu;
    boost::shared_ptr<QAction> m_extractAction;
    boost::shared_ptr<QAction> m_removeAction;
    boost::shared_ptr<QAction> m_newFolderAction;
    typedef boost::shared_ptr<QProgressDialog> SharedDialog;
    SharedDialog m_sd;
    std::set<std::string> m_populatedSet;
    void doWork(WorkType workType);
};

#endif // MAINWINDOW_H
