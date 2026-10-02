#include <obs-module.h>
#include <QApplication>
#include <cstring>
#include <iostream>
extern "C" bool obs_biblia_test_panel();

int main(int argc,char **argv)
{
    QApplication app(argc,argv);
    if(!obs_startup("en-US",nullptr,nullptr)){std::cerr<<"OBS startup failed\n";return 1;}
    const bool loaded=obs_module_load(); bool found=false;const char *id=nullptr;
    for(size_t index=0;obs_enum_input_types(index,&id);++index)
        if(std::strcmp(id,"obs_biblia_source")==0)found=true;
    const bool panel=obs_biblia_test_panel();
    obs_shutdown();
    std::cout<<(loaded&&found?"PASS":"FAIL")<<" Biblia source registers with the real OBS runtime\n";
    std::cout<<(panel?"PASS":"FAIL")<<" native panel options and transparent band rendering\n";
    return loaded&&found&&panel?0:1;
}
