#ifndef MYAPP_H
#define MYAPP_H


#include "wx/wxprec.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"
#include <wx/log.h>
#include <wx/tokenzr.h>
#include <wx/arrstr.h>
#include "wx/filefn.h"
#include "wx/dir.h"
#include <wx/init.h>	//wxInitializer
#include <wx/string.h>	//wxString
#include "wx/cmdline.h"

#include <wx/app.h>
#include <wx/cmdline.h>
#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/nonfree.hpp>

#include "boost/thread.hpp"
#include "boost/asio.hpp"


enum
{
    THREAD_QUIT  = wxID_EXIT,
    THREAD_ABOUT = wxID_ABOUT,
    THREAD_TEXT          = 101,
    THREAD_CLEAR,
    THREAD_START_THREAD  = 201,
    THREAD_START_THREADS,
    THREAD_STOP_THREAD,
    THREAD_PAUSE_THREAD,
    THREAD_RESUME_THREAD,

    THREAD_START_WORKER,
    THREAD_EXEC_MAIN,
    THREAD_START_GUI_THREAD,

    THREAD_SHOWCPUS,

    WORKER_EVENT = wxID_HIGHEST+1,   // this one gets sent from MyWorkerThread
    GUITHREAD_EVENT                  // this one gets sent from MyGUIThread
};

using namespace std;

/*
class MyThread : public wxThread
{
public:
    MyThread(wxString voc_fn01, wxString  imgs_path01, wxString result_path01);
    virtual ~MyThread(){};

    // thread execution starts here
    virtual void*  Entry();

public:
    unsigned    m_count;

private:
    string  voc_fn;
    string  imgs_path;
    string  result_path;
};
*/

class MyApp : public  wxAppConsole
{
public:
    //MyApp( MyThread  *p_thread) { mp_thread = p_thread; };
    MyApp () {};
    virtual ~MyApp() {};
    virtual bool OnInit();

    void OnWorkerEvent(wxThreadEvent& event);
    void OnQuitEvent(wxThreadEvent& event);

 private:
    //MyThread  *mp_thread;

    wxDECLARE_EVENT_TABLE();
};

wxDECLARE_APP(MyApp);

MyApp& wxGetApp();

#endif // MYAPP_H
