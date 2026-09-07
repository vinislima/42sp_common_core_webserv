/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:17 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/07 11:29:46 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>

class LocationConfig {
public:
    // "on"/"off" in the .conf set it explicitly; the directive being absent
    // from the location leaves it as INHERIT, so Response::build() falls
    // back to the parent server's value — same as root/index already do
    // with "empty = inherit".
    enum AutoindexState { AUTOINDEX_INHERIT, AUTOINDEX_ON, AUTOINDEX_OFF };

private:
    std::string                 _path;
    std::string                 _root;
    AutoindexState               _autoindex;
    std::vector<std::string>    _index;
    int                         _redirectCode;
    std::string                 _redirectUrl;
    
    // CGI and Upload variables
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
    AutoindexState getAutoindexState() const;
    std::vector<std::string> getIndex() const;
    
    std::string getCgiExt() const;
    std::string getCgiPath() const;
    std::string getUploadStore() const;

    std::vector<std::string> getAllowMethods() const;
    bool isMethodAllowed(const std::string& method) const;
};

#endif