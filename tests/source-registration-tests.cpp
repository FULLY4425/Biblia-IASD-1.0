#include <obs-module.h>
#include <QApplication>
#include <cstring>
#include <iostream>

int main(int argc,char **argv)
{
    QApplication app(argc,argv);
    if(!obs_startup("en-US",nullptr,nullptr)){std::cerr<<"OBS startup failed\n";return 1;}
    const bool loaded=obs_module_load(); bool found=false;const char *id=nullptr;
    for(size_t index=0;obs_enum_input_types(index,&id);++index)
        if(std::strcmp(id,"obs_biblia_source")==0)found=true;
    obs_shutdown();
    std::cout<<(loaded&&found?"PASS":"FAIL")<<" Biblia source registers with the real OBS runtime\n";
    return loaded&&found?0:1;
}
