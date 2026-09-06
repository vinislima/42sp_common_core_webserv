/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: yvieira- <yvieira-@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:17 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/06 12:06:18 by yvieira-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
    int                         _redirectCode;
    std::string                 _redirectUrl;
    
    // Variáveis do CGI e Upload
    std::string                 _cgiExt;
    std::string                 _cgiPath;
    std::string                 _uploadStore;
    std::vector<std::string>    _allowMethods;

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
    
    void setCgiExt(const std::string& ext);
    void setCgiPath(const std::string& path);
    void setUploadStore(const std::string& store);
    void addAllowMethod(const std::string& method);


    int getRedirectCode() const;
    std::string getRedirectUrl() const;
    std::string getPath() const;
    std::string getRoot() const;
    bool getAutoindex() const;
    std::vector<std::string> getIndex() const;
    
    std::string getCgiExt() const;
    std::string getCgiPath() const;
    std::string getUploadStore() const;

    std::vector<std::string> getAllowMethods() const;
    bool isMethodAllowed(const std::string& method) const;
};

#endif