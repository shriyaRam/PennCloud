#include "FileUtils.h"
#include "Client.h"
#include "File.h"
#include "utils.h"

// Protocols
//file_id#metadata	parent_folder_id#number_of_chunks#upload_time#file_size#filename#
//file_id#data#1
//file_id#data#2
//file_id#data#3
// folder_id#metadata val: parent_folder_id#folder_name#
// folder_id#children val: subfolder_id1#folderend#file_id1#file_id2#


// Constructor - takes in a reference to the session object for the user
FileUtils::FileUtils(Session& session_ref) : session(session_ref), kvs_client(session) {
};
// Setup the root folder for the user
bool FileUtils::setup_root_folder(std::string username) {

    // Create the root metadata
    std::string root_folder_id = "root/";
    std::string parent = "";
    std::string folder_name = "root";
    std::string metadata = create_folder_metadata(parent, folder_name);
    if (!upload_folder_metadata(username, root_folder_id, metadata)) {
        std::cerr << "Failed to upload root folder metadata" << std::endl;
        return false;
    }

    // Create the root folder content
    std::string root_folder_content = "folderend#";
    if (!upload_folder_children(username, root_folder_id, root_folder_content)) {
        std::cerr << "Failed to upload root folder content" << std::endl;
        return false;
    }
    return true;
}
// Create the metadata for a file
std::string FileUtils::create_file_metadata(std::string& data, std::string& file_id, std::string& filename, std::string& parent_folder_id) {
    int num_files = data.size() / CHUNK_SIZE;
    if (data.size() % CHUNK_SIZE != 0) {
        num_files++;
    }
    std::string upload_time = get_current_time();
    return parent_folder_id + "#" + std::to_string(num_files) + "#" + upload_time + "#" + std::to_string(data.size()) + "#" + filename + "#";
}
// Create the metadata for a folder
std::string FileUtils::create_folder_metadata(std::string& parent_folder_id, std::string& folder_name) {
    return parent_folder_id + "#" + folder_name + "#";
}
// Upload the metadata for a file
bool FileUtils::upload_file_metadata(std::string username, std::string& file_id, std::string& metadata) {
    std::string row_name = username + "#files";
    std::string col_name = file_id + "#metadata";
    return kvs_client.Put(row_name, col_name, metadata);
}
// Upload the metadata for a folder
bool FileUtils::upload_folder_metadata(std::string username, std::string& folder_id, std::string& metadata) {
    std::string row_name = username + "#folders";
    std::string col_name = folder_id + "#metadata";
    return kvs_client.Put(row_name, col_name, metadata);
}
// Upload the data for a file in chunks
bool FileUtils::chunk_upload(std::string& data, std::string& username, std::string& file_id) {
    std::string row_name = username + "#files";
    std::string col_name = file_id + "#data";
    int chunk_num = 1;
    while (data.size() > CHUNK_SIZE) {
        // std::string chunk = base64_encode(data.substr(0, CHUNK_SIZE));
        std::string chunk = data.substr(0, CHUNK_SIZE);
        data = data.substr(CHUNK_SIZE);
        if (!kvs_client.Put(row_name, col_name + "#" + std::to_string(chunk_num), chunk)) {
            std::cerr << "Failed to upload chunk " << chunk_num << std::endl;
            return false;
        }
        chunk_num++;
    }
    // deal with the last chunk
    std::string chunk = data;
    if (!kvs_client.Put(row_name, col_name + "#" + std::to_string(chunk_num), data)) {
        std::cerr << "Failed to upload chunk " << chunk_num << std::endl;
        return false;
    }
    return true;
}
// Get the metadata for a file
std::string FileUtils::get_file_metadata(std::string& username, std::string& file_id) {
    std::string row_name = username + "#files";
    std::string col_name = file_id + "#metadata";
    std::string metadata;
    if (!kvs_client.Get(row_name, col_name, metadata)) {
        std::cerr << "Failed to get file metadata" << std::endl;
        return "";
    }
    return metadata;
}
// Get the metadata for a folder
std::string FileUtils::get_folder_metadata(std::string& username, std::string& folder_id) {
    std::string row_name = username + "#folders";
    std::string col_name = folder_id + "#metadata";
    std::string metadata;
    bool success = kvs_client.Get(row_name, col_name, metadata);
    if (!success) {
        std::cerr << "Failed to get folder metadata" << std::endl;
        return "";
    }
    return metadata;
}
// Parse the metadata for a file
std::unordered_map<std::string, std::string> FileUtils::parse_file_metadata(std::string metadata) {
    std::unordered_map<std::string, std::string> metadata_map;
    size_t pos = metadata.find("#");
    metadata_map["parent_folder_id"] = metadata.substr(0, pos);
    metadata = metadata.substr(pos + 1);
    pos = metadata.find("#");
    metadata_map["num_files"] = metadata.substr(0, pos);
    metadata = metadata.substr(pos + 1);
    pos = metadata.find("#");
    metadata_map["upload_time"] = metadata.substr(0, pos);
    metadata = metadata.substr(pos + 1);
    pos = metadata.find("#");
    metadata_map["total_size"] = metadata.substr(0, pos);
    metadata = metadata.substr(pos + 1);
    pos = metadata.find("#");
    metadata_map["filename"] =metadata.substr(0, pos);
    return metadata_map;
}
// Parse the metadata for a folder
std::unordered_map<std::string, std::string> FileUtils::parse_folder_metadata(std::string metadata) {
    std::unordered_map<std::string, std::string> metadata_map;
    size_t pos = metadata.find("#");
    metadata_map["parent_folder_id"] = metadata.substr(0, pos);
    metadata = metadata.substr(pos + 1);
    pos = metadata.find("#");
    metadata_map["folder_name"] = metadata.substr(0, pos);
    return metadata_map;
}
// Assemble the chunks of a file for download
bool FileUtils::assemble_chunks(std::string& username, std::string& file_id, int chunk_num, std::string& data) {
    std::string row_name = username + "#files";
    std::string col_name = file_id + "#data";
    for (int i = 1; i <= chunk_num; i++) {
        std::string chunk;
        if (!kvs_client.Get(row_name, col_name + "#" + std::to_string(i), chunk)) {
            std::cerr << "Failed to download chunk " << i << std::endl;
            return false;
        }
        // chunk = base64_decode(chunk);
        data += chunk;
    }
    return true;
}
// Get the metadata for a folder
std::string FileUtils::get_folder_children(std::string username, std::string folder_id){
    std::string row_name = username + "#folders";
    std::string col_name = folder_id + "#children";
    std::string children;
    if (!kvs_client.Get(row_name, col_name, children)){
        return "";
    }
    return children;
}
// Upload the children of a folder (update the folder content)
bool FileUtils::upload_folder_children(std::string username, std::string folder_id, std::string children){
    std::string row_name = username + "#folders";
    std::string col_name = folder_id + "#children";
    if (!kvs_client.Put(row_name, col_name, children)){
        return false;
    }
    return true;
}
// Add a file to a folder (update the folder content)
bool FileUtils::add_file_to_folder(std::string username, std::string folder_id, std::string file_id) {
    std::string folder_children = get_folder_children(username, folder_id);
    if (folder_children.empty()) {
        folder_children = "folderend#" + file_id + "#";
    } else {
        folder_children += file_id + "#"; // add the file_id to the end
    }
    return upload_folder_children(username, folder_id, folder_children);
}
// Create a folder
bool FileUtils::upload_file(std::string username, std::string& filename, std::string& data, std::string folder_id) {
    
    std::string file_id = generate_random_id();
    std::string metadata = create_file_metadata(data, file_id, filename, folder_id);
    if (!upload_file_metadata(username, file_id, metadata)) {
        std::cerr << "Failed to upload file metadata" << std::endl;
        return false;
    }
    if (!chunk_upload(data, username, file_id)) {
        std::cerr << "Failed to upload file chunks" << std::endl;
        return false;
    }
    if (!add_file_to_folder(username, folder_id, file_id)) {
        std::cerr << "Failed to update folder content" << std::endl;
        return false;
    }
    return true;
}
// Delete a file
bool FileUtils::download_file(std::string username, std::string& file_id, std::string& data) {

    std::string metadata = get_file_metadata(username, file_id);
    if (metadata.empty()) {
        std::cerr << "Failed to download file metadata" << std::endl;
        return false;
    }
    std::unordered_map<std::string, std::string> metadata_map = parse_file_metadata(metadata);
    return assemble_chunks(username, file_id, std::stoi(metadata_map["num_files"]), data);
}
// Generate the file row for the HTML table
std::string FileUtils::generate_file_row(std::string username, std::string file_id) {
    std::string metadata = get_file_metadata(username, file_id);
    if (metadata.empty()) {
        return "";
    }
    std::unordered_map<std::string, std::string> metadata_map = parse_file_metadata(metadata);
    std::string display_name = metadata_map["filename"];
    std::string upload_time = metadata_map["upload_time"];
    size_t file_size = std::stoi(metadata_map["total_size"]);
    char html_row[1024];
    snprintf(html_row, sizeof(html_row),
             "<tr>"
             "<td>%s</td>"
             "<td>%zu bytes</td>"
             "<td>%s</td>"
             "<td>"
             "<div style=\"display: flex; align-items: center;\">"
             "<button class=\"ui tiny green button\" onclick=\"download_file('%s')\">Download</button>"
             "<button class=\"ui tiny red button\" onclick=\"delete_file('%s')\">Delete</button>"
             "<button class=\"ui tiny blue button\" onclick=\"showRenameModal('%s', '%s')\">Rename</button>"
            "<button class=\"ui tiny orange button\" onclick=\"showMoveModal('%s')\">Move</button>"
             "</div>"
             "</td>"
             "</tr>",
             display_name.c_str(), file_size, upload_time.c_str(), file_id.c_str(), file_id.c_str(), file_id.c_str(), display_name.c_str(), file_id.c_str());
    return std::string(html_row);
}
// Parse the children of a folder
void FileUtils::parse_folder_children(std::vector<std::string>& file_ids, std::vector<std::string>& folder_ids, std::string folder_content) {
    
    std::string folders = folder_content.substr(0, folder_content.find("folderend#"));
    std::string files = folder_content.substr(folder_content.find("folderend#") + 10);

    while (folders.find("#") != std::string::npos) {
        size_t pos = folders.find("#");
        std::string folder = folders.substr(0, pos);
        folder_ids.push_back(folder);
        folders = folders.substr(pos + 1);
    }

    while (files.find("#") != std::string::npos) {
        size_t pos = files.find("#");
        std::string file = files.substr(0, pos);
        file_ids.push_back(file);
        files = files.substr(pos + 1);
    }
}
// Generate the HTML table for folders
std::string FileUtils::generate_folder_row(std::string folder_id, std::string username) {

    std::string metadata = get_folder_metadata(username, folder_id);
    if (metadata.empty()) {
        return "";
    }
    std::unordered_map<std::string, std::string> metadata_map = parse_folder_metadata(metadata);
    std::string display_name = metadata_map["folder_name"];
    display_name = url_decode(display_name);
    // Generate a row for a folder
    char html_row[1024];
    snprintf(html_row, sizeof(html_row),
                "<tr>"
                "<td>"
                "<a href=\"/storage?folder=%s\">"
                "<i class=\"folder open icon\" style=\"margin-right: 5px;\"></i>%s"
                "</a>"
                "</td>" // Link to the folder
                "<td></td>" // Empty size column for folders
                "<td></td>" // Empty upload time column for folders
                "<td>"
                "<div style=\"display: flex; align-items: center;\">"
                "<button class=\"ui tiny red button\" onclick=\"delete_folder('%s')\">Delete</button>"
                "<button class=\"ui tiny blue button\" onclick=\"showRenameModal('%s', '%s')\">Rename</button>"
                "<button class=\"ui tiny orange button\" onclick=\"showMoveModal('%s')\">Move</button>"
                "</div>"
                "</td>"
                "</tr>",
                folder_id.c_str(), display_name.c_str(), folder_id.c_str(), folder_id.c_str(), display_name.c_str(), folder_id.c_str());
    return std::string(html_row);
}
// Generate the HTML row for folders
std::string FileUtils::generate_parent_folder_row(std::string username, std::string folder_id){
    // generate a row that represents the parent folder
    if (folder_id != "root/") {
        std::string folder_metadata = get_folder_metadata(username, folder_id);
        std::string parent_folder_id = folder_metadata.substr(0, folder_metadata.find("#"));
        return  
            "<tr>"
            "<td>"
            "<a href=\"/storage?folder=" + parent_folder_id + "\">"
            "<i class=\"folder open icon\" style=\"margin-right: 5px;\"></i>..."
            "</a>"
            "</td>";
            "<td></td>"
            "<td></td>" 
            "<td></td>" 
            "</tr>";
    }
    return "";
}
// Generate the HTML table for files
std::string FileUtils::generate_file_table(std::string username, std::string folder_id) {

    std::vector<std::string> file_ids;
    std::vector<std::string> subfolder_ids;
    std::string folder_content = get_folder_children(username, folder_id);
    if (folder_content.empty()) {
        return "";
    }
    parse_folder_children(file_ids, subfolder_ids, folder_content);
    // create the display for folders
    std::string folder_rows;

    // a link to the last layer of the parent folder
    folder_rows += generate_parent_folder_row(username, folder_id);

    for (const auto& folder_id : subfolder_ids) {
        std::string row = generate_folder_row(folder_id, username);
        if (!row.empty()) {
            folder_rows += row;
        }
    }
    // create the display for files
    std::string file_rows;
    for (const auto& file_id : file_ids) {
        std::string row = generate_file_row(username, file_id);
        if (!row.empty()) {
            file_rows += row;
        }
    }
    return folder_rows + file_rows;
}
// Create a folder for the user
bool FileUtils::create_folder(std::string username, std::string folder_name, std::string parent_folder_id) {

    std::string folder_id = generate_random_id();
    std::string metadata = create_folder_metadata(parent_folder_id, folder_name);
    if (!upload_folder_metadata(username, folder_id, metadata)) {
        std::cerr << "Failed to upload folder metadata" << std::endl;
        return false;
    } 
    // create the folder content
    std::string folder_content = "folderend#";
    if (!upload_folder_children(username, folder_id, folder_content)) {
        std::cerr << "Failed to upload folder content" << std::endl;
        return false;
    }
    // update the children of the parent folder
    std::string parent_folder_content = get_folder_children(username, parent_folder_id);
    if (parent_folder_content.empty()) {
        parent_folder_content = folder_id + "#folderend#";
    } else {
        parent_folder_content = folder_id + "#" + parent_folder_content; // folders are in the front
    }
    if (!upload_folder_children(username, parent_folder_id, parent_folder_content)) {
        std::cerr << "Failed to update parent folder content" << std::endl;
        return false;
    }
    return true;   
}
// Delete the chunks of a file
bool FileUtils::delete_chunks(std::string username, std::string file_id, int num_files){
    std::string row_name = username + "#files";
    std::string col_name = file_id + "#data";
    for (int i = 1; i <= num_files; i++) {
        if (!kvs_client.Del(row_name, col_name + "#" + std::to_string(i))) {
            std::cerr << "Failed to delete chunk " << i << std::endl;
            return false;
        }
    }
    return true;
}
// Delete the metadata of a file or folder
bool FileUtils::delete_metadata(std::string username, std::string id, bool is_folder){
    std::string row_name;
    if (is_folder){
        row_name = username + "#folders";
    } else {
        row_name = username + "#files";
    }
    return kvs_client.Del(row_name, id + "#metadata");
}
// Delete a file from a folder
bool FileUtils::delete_file_from_folder(std::string username, std::string file_id) {
    std::string metadata = get_file_metadata(username, file_id);
    if (metadata.empty()) {
        return false;
    }
    std::unordered_map<std::string, std::string> metadata_map = parse_file_metadata(metadata);
    std::string parent_folder_id = metadata_map["parent_folder_id"];
    std::string folder_content = get_folder_children(username, parent_folder_id);
    if (folder_content.empty()) {
        return false;
    }
    folder_content = folder_content.replace(folder_content.find(file_id), file_id.size() + 1, "");
    return upload_folder_children(username, parent_folder_id, folder_content);
}
// Delete a file
bool FileUtils::delete_file(std::string username, std::string& file_id){
    // Get the file metadata and parse
    std::unordered_map<std::string, std::string> metadata_map = parse_file_metadata(get_file_metadata(username, file_id));
    int num_files = std::stoi(metadata_map["num_files"]);
    // Delete all the chunks
    if (!delete_chunks(username, file_id, num_files)) {
        std::cerr << "Failed to delete chunks" << std::endl;
        return false;
    }
    // delete the file from folder content
    if (!delete_file_from_folder(username, file_id)) {
        std::cerr << "Failed to delete file from folder" << std::endl;
        return false;
    }
    // delete metadata
    if (!delete_metadata(username, file_id, false)) {
        std::cerr << "Failed to delete metadata" << std::endl;
        return false;
    }
    return true;
}
// Delete a folder
bool FileUtils::delete_folder(std::string username, std::string folder_id) {
    // check if the folder exists
    std::string metadata = get_folder_metadata(username, folder_id);
    if (metadata.empty()) {
        return false;
    }
    // delete all the folders/files in the folder
    std::string folder_content = get_folder_children(username, folder_id);
    std::vector<std::string> file_ids;
    std::vector<std::string> folder_ids;
    parse_folder_children(file_ids, folder_ids, folder_content);
    for (auto& file_id : file_ids) {
        if (!delete_file(username, file_id)) {
            std::cerr << "Failed to delete file" << std::endl;
            return false;
        }
    }
    for (auto& subfolder_id : folder_ids) {
        if (!delete_folder(username, subfolder_id)) {
            std::cerr << "Failed to delete folder" << std::endl;
            return false;
        }
    }
    // delete the folder metadata
    if (!delete_metadata(username, folder_id, true)) {
        std::cerr << "Failed to delete folder metadata" << std::endl;
        return false;
    }
    // delete the folder content
    if (!delete_metadata(username, folder_id + "#children", true)) {
        std::cerr << "Failed to delete folder content" << std::endl;
        return false;
    }
    // delete the folder from the folder content /parent/folder/
    std::string parent_folder_id = metadata.substr(0, metadata.find("#"));
    std::string parent_folder_content = get_folder_children(username, parent_folder_id);
    parent_folder_content = parent_folder_content.replace(parent_folder_content.find(folder_id), folder_id.size() + 1, "");
    if (!upload_folder_children(username, parent_folder_id, parent_folder_content)) {
        std::cerr << "Failed to update parent folder content" << std::endl;
        return false;
    }
    return true;
}
// Rename a file
bool FileUtils::rename_file(std::string username, std::string file_id, std::string new_filename){

    // update the metadata
    std::string metadata = get_file_metadata(username, file_id);
    if (metadata.empty()) {
        return false;
    }
    // update the metadata with the new filename
    std::unordered_map<std::string, std::string> metadata_map = parse_file_metadata(metadata);
    metadata_map["filename"] = new_filename;
    metadata = metadata_map["parent_folder_id"] + "#" + metadata_map["num_files"] + "#" + metadata_map["upload_time"] + "#" + metadata_map["total_size"] + "#" + new_filename + "#";
    if (!upload_file_metadata(username, file_id, metadata)) {
        std::cerr << "Failed to update metadata" << std::endl;
        return false;
    }
    return true;
}
// Rename a folder
bool FileUtils::rename_folder(std::string username, std::string folder_id, std::string new_folder_name) {
    
    // update the metadata
    std::string metadata = get_folder_metadata(username, folder_id);
    if (metadata.empty()) {
        return false;
    }
    // update the metadata with the new folder name
    std::unordered_map<std::string, std::string> metadata_map = parse_folder_metadata(metadata);
    metadata = metadata_map["parent_folder_id"] + "#" + new_folder_name + "#";
    if (!upload_folder_metadata(username, folder_id, metadata)) {
        std::cerr << "Failed to update metadata" << std::endl;
        return false;
    }
    return true;
}
// Generate a random id for a file or folder
std::string FileUtils::generate_folder_tree_helper(std::string& username, std::string& folder_id) {

    // Get metadata for the folder
    std::string metadata = get_folder_metadata(username, folder_id);
    if (metadata.empty()) return "";

    std::unordered_map<std::string, std::string> metadata_map = parse_folder_metadata(metadata);
    std::string folder_name = metadata_map["folder_name"];

    // Start HTML for this folder
    std::string html = "<div class=\"item\">\n";
    html += "  <i class=\"folder icon\"></i>\n"; // Folder icon
    html += "  <div class=\"content\">\n";
    html += "    <div class=\"header folder-select\" data-value=\"" + folder_id + "\">" + url_decode(folder_name) + "</div>\n";

    // Get children of the folder
    std::string folder_content = get_folder_children(username, folder_id);
    if (!folder_content.empty()) {
        std::vector<std::string> file_ids;
        std::vector<std::string> subfolder_ids;
        parse_folder_children(file_ids, subfolder_ids, folder_content);

        // If there are subfolders, add them as a nested list
        if (!subfolder_ids.empty()) {
            html += "    <div class=\"list\">\n";
            for (auto& subfolder_id : subfolder_ids) {
                html += generate_folder_tree_helper(username, subfolder_id); // Recursive call
            }
            html += "    </div>\n";
        }
    }

    html += "  </div>\n"; // Close content
    html += "</div>\n";   // Close item
    return html;
}
//  Generate the folder tree for the user
std::string FileUtils::generate_folder_tree(std::string& username) {
    std::string start = "root/";
    return generate_folder_tree_helper(username, start);
}
// Move a file
bool FileUtils::move_file(std::string username, std::string file_id, std::string destination_folder){
    // Get the metadata of the file
    std::string metadata = get_file_metadata(username, file_id);
    if (metadata.empty()) {
        return false;
    }
    // Get the parent folder of the file
    std::unordered_map<std::string, std::string> metadata_map = parse_file_metadata(metadata);
    std::string parent_folder_id = metadata_map["parent_folder_id"];
    if (parent_folder_id == destination_folder) {
        return true;
    }
    // Get the destination folder content
    std::string destination_folder_content = get_folder_children(username, destination_folder);
    if (destination_folder_content.empty()) {
        return false;
    }
    // Add the file to the destination folder
    destination_folder_content += file_id + "#";
    // Update the destination folder content
    if (!upload_folder_children(username, destination_folder, destination_folder_content)) {
        return false;
    }
    // Update the content of parent folder
    std::string parent_folder_content = get_folder_children(username, parent_folder_id);
    if (parent_folder_content.empty()) {
        return false;
    }
    parent_folder_content = parent_folder_content.replace(parent_folder_content.find(file_id), file_id.size() + 1, "");
    // Update the parent folder content
    if (!upload_folder_children(username, parent_folder_id, parent_folder_content)) {
        return false;
    }
    // Update the metadata of the file
    metadata_map["parent_folder_id"] = destination_folder;
    metadata = metadata_map["parent_folder_id"] + "#" + metadata_map["num_files"] + "#" + metadata_map["upload_time"] + "#" + metadata_map["total_size"] + "#" + metadata_map["filename"] + "#";
    if (!upload_file_metadata(username, file_id, metadata)) {
        return false;
    }
    return true;
}
// Move a folder
bool FileUtils::move_folder(std::string username, std::string folder_id, std::string destination_folder){
    // Get the metadata of the folder
    std::string metadata = get_folder_metadata(username, folder_id);
    if (metadata.empty()) {
        return false;
    }
    // Get the parent folder of the folder
    std::unordered_map<std::string, std::string> metadata_map = parse_folder_metadata(metadata);
    std::string parent_folder_id = metadata_map["parent_folder_id"];
    if (parent_folder_id == destination_folder) {
        return true;
    }
    // Get the destination folder content
    std::string destination_folder_content = get_folder_children(username, destination_folder);
    if (destination_folder_content.empty()) {
        return false;
    }
    // Add the folder to the destination folder
    destination_folder_content = folder_id + "#" + destination_folder_content;
    // Update the destination folder content
    if (!upload_folder_children(username, destination_folder, destination_folder_content)) {
        return false;
    }
    // Update the content of parent folder
    std::string parent_folder_content = get_folder_children(username, parent_folder_id);
    if (parent_folder_content.empty()) {
        return false;
    }
    parent_folder_content = parent_folder_content.replace(parent_folder_content.find(folder_id), folder_id.size() + 1, "");
    // Update the parent folder content
    if (!upload_folder_children(username, parent_folder_id, parent_folder_content)) {
        return false;
    }
    // Update the metadata of the folder
    metadata_map["parent_folder_id"] = destination_folder;
    metadata = metadata_map["parent_folder_id"] + "#" + metadata_map["folder_name"] + "#";
    if (!upload_folder_metadata(username, folder_id, metadata)) {
        return false;
    }
    return true;
}
