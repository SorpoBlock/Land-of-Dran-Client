#include "code/utility/fileOpeartions.h"

bool updateableFile(std::string path)
{
    if(path.length() < 3)
        return false;

    if(path.substr(path.length()-4,4) == ".exe")
        return true;

    if(path.substr(path.length()-5,5) == ".glsl")
        return true;

    if(path.substr(path.length()-4,4) == ".dll")
        return true;

    if(path.substr(0,6) == ".\\gui\\")
        return true;

    return false;
}

unsigned int getFileChecksum(const char *filePath)
{
    std::ifstream file(filePath,std::ios::in | std::ios::binary);
    if(!file.is_open())
        return 0;

    const unsigned int bufSize = 2048;
    char buffer[bufSize];
    unsigned int ret = 0;
    while(!file.eof())
    {
        file.read(buffer,bufSize);
        ret = CRC::Calculate(buffer,file.gcount(),CRC::CRC_32(),ret);
    }

    file.close();
    return ret;
}

void replaceAll(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());  // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while(std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    source.swap(newString);
}

std::string charToHex(unsigned char in)
{
    std::stringstream stream;
    stream << std::hex << (int)in;
    std::string result( stream.str() );
    if(result.length() == 0)
        return "00";
    else if(result.length() == 1)
        return "0" + result;
    else
        return result;
}

long GetFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

std::string lowercase(std::string in)
{
    std::string ret = "";
    for(int a = 0; a<in.length(); a++)
    {
        if(in[a] >= 'A' && in[a] <= 'Z')
            ret += tolower(in[a]);
        else if(in[a] >= 'a' && in[a] <= 'z')
            ret += in[a];
        else if(in[a] >= '0' && in[a] <= '9')
            ret += in[a];
        else if(in[a] == '-')
            ret += in[a];
        else if(in[a] == ' ')
            ret += in[a];
    }
    return ret;
}

void split(const std::string &s,std::vector<std::string> &elems)
{
    std::stringstream wordSplitter(s);
    elems.clear();
    std::string buffer;
    while (wordSplitter >> buffer)
    {
        if(buffer.length() > 0 && buffer != " ")
            elems.push_back(buffer);
    }
}

//Gets the file name and extension from a full file path
//e.g. "assets/bob/bob.png" as filepath returns "bob.png"
std::string getFileFromPath(std::string in)
{
    while(in.find("/") != std::string::npos)
        in = in.substr(in.find("/")+1,(in.length() - in.find("/"))+1);
    while(in.find("\\") != std::string::npos)
        in = in.substr(in.find("\\")+1,(in.length() - in.find("\\"))+1);
    return in;
}

//Gets the folder from a full file path
//e.g. "assets/bob/bob.png" as filepath returns "assets/bob/"
std::string getFolderFromPath(std::string in)
{
    if(in.find("/") == std::string::npos)
        return "";
    std::string file = getFileFromPath(in);
    if(in.length() > file.length())
        in = in.substr(0,in.length() - file.length());
    return in;
}

//Adds a suffix to the end of a file name before the extension
//E.g. in = "test/bob.png", suffix = "_a" will return "test/bob_a.png"
std::string addSuffixToFile(std::string in,std::string suffix)
{
    int lastPeriod = in.find(".",0);
    while(in.find(".",lastPeriod+1) != std::string::npos)
        lastPeriod = in.find(".",lastPeriod+1);
    return in.substr(0,lastPeriod) + suffix + in.substr(lastPeriod,in.length() - lastPeriod);
}

bool doesFileExist(std::string filePath)
{
    std::ifstream test(filePath.c_str());
    if(test.is_open())
    {
        test.close();
        return true;
    }
    return false;
}
