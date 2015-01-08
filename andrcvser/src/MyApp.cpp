#include "MyApp.h"

wxBEGIN_EVENT_TABLE(MyApp,  wxAppConsole)
EVT_THREAD(WORKER_EVENT, MyApp::OnWorkerEvent)
EVT_THREAD(THREAD_QUIT,  MyApp::OnQuitEvent)
wxEND_EVENT_TABLE()

/*
MyThread::MyThread(wxString voc_fn01, wxString  imgs_path01, wxString result_path01)
{
    voc_fn = voc_fn01.mb_str();
    imgs_path = imgs_path01.mb_str();
    result_path = result_path01.mb_str();
}
*/

bool MyApp::OnInit()
{
    if ( !wxAppConsole::OnInit() )
        return false;

    /*
    if ( mp_thread->Create() != wxTHREAD_NO_ERROR )
    {
        std::cout << "Can't create thread! " << std::endl;
    }

    mp_thread->Run();
    */

    return true;
}

void MyApp::OnWorkerEvent(wxThreadEvent& event)
{
    wxString  str_txt = event.GetString();

    //std::cout << "thread event : " << str_txt << std::endl;
    std::cout << str_txt << std::endl;
}

void MyApp::OnQuitEvent(wxThreadEvent& event)
{
    std::cout << "Quit" << std::endl;
    exit(0);
}

MyApp& wxGetApp() { return *(MyApp *)wxTheApp; }
