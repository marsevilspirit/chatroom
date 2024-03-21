#include "group.h"

Group::Group(const std::string& name):groupName(name)
{
    std::cout << "Group " << name << " created.\n"; 
}

Group::~Group()
{

}
