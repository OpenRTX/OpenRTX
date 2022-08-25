
#include "SystemMap.h"

using namespace std;

#ifdef WITH_PROCESSES

namespace miosix {

SystemMap& SystemMap::instance()
{
    static SystemMap singleton;
    return singleton;
}

void SystemMap::addElfProgram(const char* name, const unsigned int *elf, unsigned int size)
{
    string sName(name);
    if(mPrograms.find(sName) == mPrograms.end())
        mPrograms.insert(make_pair(sName, make_pair(elf, size)));
}

void SystemMap::removeElfProgram(const char* name)
{
    string sName(name);
    ProgramsMap::iterator it = mPrograms.find(sName);

    if(it != mPrograms.end())
        mPrograms.erase(it);
}

pair<const unsigned int*, unsigned int>  SystemMap::getElfProgram(const char* name) const
{
    ProgramsMap::const_iterator it = mPrograms.find(string(name));

    if(it == mPrograms.end())
        return make_pair<const unsigned int*, unsigned int>(0, 0);

    return it->second;
}

unsigned int SystemMap::getElfCount() const
{
    return mPrograms.size();
}

} //namespace miosix

#endif //WITH_PROCESSES
