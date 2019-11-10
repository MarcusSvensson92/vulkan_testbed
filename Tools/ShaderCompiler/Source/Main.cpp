#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>
 
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
#endif
 
struct ShaderFile
{
    std::string Path;
    uint64_t    Hash;
};
void AddShaderFile(const std::string& dir, const std::string& filename, std::vector<ShaderFile>& files)
{
    const char* extension = strrchr(filename.c_str(), '.');
 
    const char* shader_extensions[] =
    {
        ".vert",
        ".tesc",
        ".tese",
        ".geom",
        ".frag",
        ".comp",
        ".mesh",
        ".task",
        ".rgen",
        ".rint",
        ".rahit",
        ".rchit",
        ".rmiss",
        ".rcall",
    };
    const uint32_t num_shader_extensions = static_cast<uint32_t>(sizeof(shader_extensions) / sizeof(*shader_extensions));
 
    bool is_shader = false;
    for (uint32_t i = 0; i < num_shader_extensions; ++i)
    {
        if (strcmp(extension, shader_extensions[i]) == 0)
        {
            is_shader = true;
            break;
        }
    }
 
    if (is_shader)
    {
        std::ifstream file(dir + filename);
        if (file.is_open())
        {
            uint64_t hash = 0;
            char c;
            while (file.get(c))
            {
                // http://www.eecs.harvard.edu/margo/papers/usenix91/paper.ps
                hash = c + (hash << 6) + (hash << 16) - hash;
            }
 
            file.close();
 
            ShaderFile entry;
            entry.Path = filename;
            entry.Hash = hash;
            files.emplace_back(entry);
        }
    }
}
 
static void GatherShaderFiles(const std::string& in_dir, std::vector<ShaderFile>& files)
{
#ifdef _WIN32
    WIN32_FIND_DATA item;
    HANDLE search_handle = FindFirstFile((in_dir + "*").c_str(), &item);
    if (search_handle == INVALID_HANDLE_VALUE)
        return;
    do
    {
        if (item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        AddShaderFile(in_dir, item.cFileName, files);
    } while (FindNextFile(search_handle, &item));
    FindClose(search_handle);
#else
    DIR* dir = opendir(in_dir.c_str());
    if (dir == NULL)
        return;
    while (dirent* ent = readdir(dir))
    {
        if (ent->d_type == DT_DIR)
            continue;
        AddShaderFile(in_dir, ent->d_name, files);
    }
    closedir(dir);
#endif
}
 
int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Incorrect number of arguments");
        return 1;
    }
   
    std::string in_dir = argv[1];
    std::string out_dir = argv[2];
   
    std::vector<ShaderFile> shader_files;
    GatherShaderFiles(in_dir, shader_files);
 
    std::stringstream new_file_content;
 
    std::string file_list_path = out_dir + "ShaderFileList.txt";
    std::ifstream old_file(file_list_path);
    if (old_file.is_open())
    {
        std::string path;
        uint64_t hash;
        while (old_file >> path >> hash)
        {
            for (std::vector<ShaderFile>::iterator it = shader_files.begin(); it != shader_files.end(); ++it)
            {
                if (it->Path == path)
                {
                    if (it->Hash == hash)
                    {
                        new_file_content << it->Path << " " << it->Hash << std::endl;
                        shader_files.erase(it);
                    }
                    break;
                }
            }
        }
 
        old_file.close();
    }
 
    bool any_compile_errors = false;
    for (const ShaderFile& file : shader_files)
    {
#ifdef _WIN32
	std::string executable_path = "%VULKAN_SDK%/Bin/glslangValidator.exe";
#else
	std::string executable_path = "${VULKAN_SDK}/bin/glslangValidator";
#endif
        std::string command = executable_path + " -V -o " + out_dir + file.Path + " " + in_dir + file.Path + " --target-env vulkan1.1";
        int status = system(command.c_str());
        if (status == 0)
        {
            new_file_content << file.Path << " " << file.Hash << std::endl;
        }
        else
        {
            any_compile_errors = true;
        }
    }
 
    std::ofstream new_file(file_list_path);
    if (new_file.is_open())
    {
        new_file << new_file_content.rdbuf();
        new_file.close();
    }
 
    return any_compile_errors ? 1 : 0;
}

