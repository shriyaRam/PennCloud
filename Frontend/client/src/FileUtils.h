#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>
#include <unordered_map>
#include <vector>
#include "Client.h"
#include "Session.h"    

class FileUtils{
    private:
        // Constants
        const size_t CHUNK_SIZE = 1024 * 1024 * 3.9; 
        // this is the size of "file" -> when converted to base64, it will be 4/3 times the size of the original file - so in fact we are passing almost 4 MB of data
        Session& session;
        Client kvs_client;

    public:

        // Constructor
        FileUtils(Session& session_ref);

        bool setup_root_folder(std::string username);

        // Metadata Creation
        std::string create_file_metadata(std::string& data, std::string& file_id, std::string& filename, std::string& parent_folder_id);
        std::string create_folder_metadata(std::string& parent_folder_id, std::string& folder_name);

        // Metadata Upload
        bool upload_file_metadata(std::string username, std::string& file_id, std::string& metadata);
        bool upload_folder_metadata(std::string username, std::string& folder_id, std::string& metadata);

        // Chunk Handling
        bool chunk_upload(std::string& data, std::string& username, std::string& file_id);
        bool assemble_chunks(std::string& username, std::string& file_id, int chunk_num, std::string& data);

        // Metadata Retrieval
        std::string get_file_metadata(std::string& username, std::string& file_id);
        std::string get_folder_metadata(std::string& username, std::string& folder_id);

        // Folder Content Management
        std::string get_folder_children(std::string username, std::string folder_id);
        bool upload_folder_children(std::string username, std::string folder_id, std::string children);
        bool add_file_to_folder(std::string username, std::string folder_id, std::string file_id);

        // Metadata Parsing
        std::unordered_map<std::string, std::string> parse_file_metadata(std::string metadata);
        std::unordered_map<std::string, std::string> parse_folder_metadata(std::string metadata);
        void parse_folder_children(std::vector<std::string>& file_ids, std::vector<std::string>& folder_ids, std::string folder_content);

        // File Operations
        bool upload_file(std::string username, std::string& filename, std::string& data, std::string folder_id);
        bool download_file(std::string username, std::string& file_id, std::string& data);
        bool delete_file(std::string username, std::string& file_id);
        bool delete_file_from_folder(std::string username, std::string file_id);
        bool move_file(std::string username, std::string folder_id, std::string destination_folder);

        // Folder Operations
        bool create_folder(std::string username, std::string folder_name, std::string parent_folder_id);
        bool delete_folder(std::string username, std::string folder_id);
        bool rename_folder(std::string username, std::string folder_id, std::string new_folder_name);
        bool move_folder(std::string username, std::string folder_id, std::string destination_folder);

        // File Renaming
        bool rename_file(std::string username, std::string file_id, std::string new_filename);

        // HTML Row Generation
        std::string generate_file_row(std::string username, std::string file_id);
        std::string generate_folder_row(std::string folder_id, std::string username);
        std::string generate_file_table(std::string username, std::string folder_id);
        std::string generate_parent_folder_row(std::string username, std::string folder_id);
        std::string generate_folder_tree_helper(std::string& username, std::string& folder_id);
        std::string generate_folder_tree(std::string& username);

        // Utility Functions
        bool delete_chunks(std::string username, std::string file_id, int num_files);
        bool delete_metadata(std::string username, std::string id, bool is_folder);
};



#endif // FILEUTILS_H
