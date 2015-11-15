#include "MyApp.h"

wxBEGIN_EVENT_TABLE(MyApp,  wxAppConsole)
EVT_THREAD(WORKER_EVENT, MyApp::OnWorkerEvent)
EVT_THREAD(WORKER_CLS, MyApp::OnWorkerCls)
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

    cur_line = 0;

    /*
    if ( mp_thread->Create() != wxTHREAD_NO_ERROR )
    {
        std::cout << "Can't create thread! " << std::endl;
    }

    mp_thread->Run();
    */

    return true;
}

void MyApp::OnWorkerCls(wxThreadEvent& event)
{
    cur_line = 0;
    //erase();
    //refresh();
}

void MyApp::OnWorkerEvent(wxThreadEvent& event)
{
    wxString  wstr_txt = event.GetString();
    string    str_txt;
    str_txt = wstr_txt.mb_str();

    /*
    int  max_row = LINES-1;
    int  max_col = COLS-1;

    mvwprintw(stdscr, cur_line++, 0, str_txt.c_str());

    if (cur_line > max_row)
    {
        cur_line = max_row;
    }

    refresh();
    */

    std::cout << str_txt << std::endl;
}

void MyApp::OnQuitEvent(wxThreadEvent& event)
{
    //endwin();

    std::cout << "Quit" << std::endl;

    exit(0);
}

MyApp& wxGetApp() { return *(MyApp *)wxTheApp; }
