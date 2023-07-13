#ifndef FILEOPEARTIONS_H_INCLUDED
#define FILEOPEARTIONS_H_INCLUDED

#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <bits/stdc++.h>
#include <sys/stat.h>

void replaceAll(std::string& source, const std::string& from, const std::string& to);

std::string charToHex(unsigned char in);

long GetFileSize(std::string filename);

std::string lowercase(std::string in);

void split(const std::string &s,std::vector<std::string> &elems);

//Gets the file name and extension from a full file path
//e.g. "assets/bob/bob.png" as filepath returns "bob.png"
std::string getFileFromPath(std::string in);

//Gets the folder from a full file path
//e.g. "assets/bob/bob.png" as filepath returns "assets/bob/"
std::string getFolderFromPath(std::string in);

//Adds a suffix to the end of a file name before the extension
//E.g. in = "test/bob.png", suffix = "_a" will return "test/bob_a.png"
std::string addSuffixToFile(std::string in,std::string suffix);

bool doesFileExist(std::string filePath);

#endif // FILEOPEARTIONS_H_INCLUDED
