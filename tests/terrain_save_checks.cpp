#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<unsigned char> Read(const char* path)
{
    std::ifstream in(path,std::ios::binary);
    if(!in) throw std::runtime_error(std::string("Cannot open ")+path);
    return {std::istreambuf_iterator<char>(in),{}};
}
uint32_t Word(const std::vector<unsigned char>& b,size_t offset)
{
    uint32_t value=0;std::memcpy(&value,b.data()+offset,4);return value;
}
int main(int argc,char** argv)
{
    if(argc!=8) { std::cerr<<"Usage: terrain_save_checks before after dds|collision x0 z0 x1 z1\n";return 2; }
    try {
        const auto before=Read(argv[1]),after=Read(argv[2]);
        const bool dds=std::string(argv[3])=="dds";
        const int x0=std::stoi(argv[4]),z0=std::stoi(argv[5]),x1=std::stoi(argv[6]),z1=std::stoi(argv[7]);
        size_t offset=0,unit=2;int width=2048,depth=2048;
        if(before.size()!=after.size()) throw std::runtime_error("Saved file length changed");
        if(dds) {
            if(before.size()<128 || Word(before,0)!=0x20534444 || Word(before,84)!=114) throw std::runtime_error("Expected legacy DDS R32F format");
            width=static_cast<int>(Word(before,16));depth=static_cast<int>(Word(before,12));offset=128;unit=4;
            if(!std::equal(before.begin(),before.begin()+128,after.begin())) throw std::runtime_error("DDS header changed");
        }
        if(width<=0 || depth<=0 || before.size()!=offset+static_cast<size_t>(width)*static_cast<size_t>(depth)*unit)
            throw std::runtime_error("Unexpected native terrain file dimensions");
        size_t changed=0,outside=0;int minX=width,minZ=depth,maxX=-1,maxZ=-1;
        for(int z=0;z<depth;++z) for(int x=0;x<width;++x) {
            // DDS texture is row-major Z/X; native collision height array is X/Z.
            const auto index=dds?z*width+x:x*depth+z;
            const auto at=offset+static_cast<size_t>(index)*unit;
            if(std::memcmp(before.data()+at,after.data()+at,unit)!=0) {
                ++changed;minX=std::min(minX,x);maxX=std::max(maxX,x);minZ=std::min(minZ,z);maxZ=std::max(maxZ,z);
                if(x<x0 || x>x1 || z<z0 || z>z1) ++outside;
            }
        }
        std::cout<<"{\"kind\":\""<<(dds?"DDS R32F":"native collision uint16")<<"\",\"changed\":"<<changed
            <<",\"outside\":"<<outside<<",\"minX\":"<<minX<<",\"maxX\":"<<maxX<<",\"minZ\":"<<minZ<<",\"maxZ\":"<<maxZ<<"}\n";
        return changed>0 && outside==0?0:1;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
