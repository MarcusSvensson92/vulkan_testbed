#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include <Windows.h>

struct ShaderFile
{
	std::string	Path;
	uint64_t	Hash;
};

static void GatherShaderFiles(const std::string& in_dir, const std::string& out_dir, const std::string& path, WIN32_FIND_DATA* item, std::vector<ShaderFile>& files)
{
	HANDLE search_handle = FindFirstFile((in_dir + path + "*").c_str(), item);
	if (search_handle == INVALID_HANDLE_VALUE)
		return;
	do
	{
		if (strcmp(item->cFileName, ".") == 0 || strcmp(item->cFileName, "..") == 0)
			continue;
		if (item->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		const char* extension = strrchr(item->cFileName, '.');

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
			std::ifstream file(in_dir + path + item->cFileName);
			if (!file.is_open())
				continue;

			uint64_t hash = 0;
			char c;
			while (file.get(c))
			{
				// http://www.eecs.harvard.edu/margo/papers/usenix91/paper.ps
				hash = c + (hash << 6) + (hash << 16) - hash;
			}

			file.close();

			ShaderFile entry;
			entry.Path = path + item->cFileName;
			entry.Hash = hash;
			files.emplace_back(entry);
		}

	} while (FindNextFile(search_handle, item));

	FindClose(search_handle);
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

	WIN32_FIND_DATA item;
	GatherShaderFiles(in_dir, out_dir, "", &item, shader_files);

	std::stringstream new_file_content;
	for (const ShaderFile& file : shader_files)
	{
		new_file_content << file.Path << " " << file.Hash << std::endl;
	}

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
						shader_files.erase(it);
					break;
				}
			}
		}

		old_file.close();
	}

	for (const ShaderFile& file : shader_files)
	{
		std::string command = "%VULKAN_SDK%/Bin/glslangValidator.exe -V -o " + out_dir + file.Path + " " + in_dir + file.Path + " --target-env vulkan1.1";
		system(command.c_str());
	}

	std::ofstream new_file(file_list_path);
	if (new_file.is_open())
	{
		new_file << new_file_content.rdbuf();
		new_file.close();
	}

    return 0;
}