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
    std::string                 _cgiExt;
    std::string                 _cgiPath; 
	std::string					_path;
	std::string					_root;
	bool						_autoindex;
	std::vector<std::string>	_index;
	int							_redirectCode;
	std::string					_redirectUrl;
	std::string					_uploadStore;

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
	void setRedirect(int code, const std::string& url);
	void setUploadStore(const std::string& path);
	int getRedirectCode() const;
	std::string getRedirectUrl() const;

    std::string                 getPath() const;
    std::string                 getRoot() const;
    bool                        getAutoindex() const;
    std::vector<std::string>    getIndex() const;

    void setCgiExt(const std::string& ext);
    void setCgiPath(const std::string& path);
    std::string getCgiExt() const;
    std::string getCgiPath() const;
    
	std::vector<std::string>	getIndex() const;
	std::string getUploadStore() const;
};

#endif
