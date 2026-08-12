#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>

class LocationConfig {
private:
    std::string                 _path;      
    std::string                 _root;      
    bool                        _autoindex; 
    std::vector<std::string>    _index;     

public:
    LocationConfig();
    LocationConfig(const std::string& path);
    LocationConfig(const LocationConfig& src);
    LocationConfig& operator=(const LocationConfig& rhs);
    ~LocationConfig();

    void setPath(const std::string& path);
    void setRoot(const std::string& root);
    void setAutoindex(bool autoindex);
    void addIndex(const std::string& index);

    std::string                 getPath() const;
    std::string                 getRoot() const;
    bool                        getAutoindex() const;
    std::vector<std::string>    getIndex() const;
};

#endif