/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vinda-si <vinda-si@student.42sp.org.br>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/06 12:06:17 by yvieira-          #+#    #+#             */
/*   Updated: 2026/09/07 16:18:21 by vinda-si         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>

/**
 * @class LocationConfig
 * @brief Configuration for a single "location { ... }" block inside a server.
 *
 * Holds the directives that can be overridden per-route (root, autoindex,
 * index, client_max_body_size, redirects, CGI, uploads, allowed methods).
 * Directives left unset in the .conf fall back to the owning ServerConfig's
 * value — see @ref AutoindexState and hasClientMaxBodySize() for how each
 * field represents "not set" vs. an explicit value.
 */
class LocationConfig {
public:
	/**
	 * @brief Tri-state autoindex flag, distinguishing "not set here" from an explicit on/off.
	 *
	 * A plain bool can't represent "inherit from the server", since both
	 * true and false are valid explicit values. AUTOINDEX_INHERIT is the
	 * default when the location's block never mentions "autoindex" at all;
	 * Response::build() then falls back to the parent server's setting,
	 * the same way it does for root/index using an empty string.
	 */
	enum AutoindexState { AUTOINDEX_INHERIT, AUTOINDEX_ON, AUTOINDEX_OFF };

private:
	std::string					_path;
	std::string					_root;
	AutoindexState				_autoindex;
	std::vector<std::string>	_index;
	int							_redirectCode;
	std::string					_redirectUrl;

	size_t						_clientMaxBodySize;
	bool						_hasClientMaxBodySize; ///< True once client_max_body_size was set for this location (0 alone can't mean "unset", since 0 is a valid "no limit" value).

	std::string					_cgiExt;
	std::string					_cgiPath;
	std::string					_uploadStore;
	std::vector<std::string>	_allowMethods;

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
	/// @brief Sets the body size limit and marks it as explicitly configured (see hasClientMaxBodySize()).
	void setClientMaxBodySize(size_t size);

	void setCgiExt(const std::string& ext);
	void setCgiPath(const std::string& path);
	void setUploadStore(const std::string& store);
	void addAllowMethod(const std::string& method);

	/// @return The HTTP status code for this location's redirect, or 0 if none is configured.
	int getRedirectCode() const;
	/// @return The target URL for this location's redirect (only meaningful if getRedirectCode() != 0).
	std::string getRedirectUrl() const;
	/// @return The location's path prefix, as written in the .conf (e.g. "/uploads").
	std::string getPath() const;
	/// @return The location's root directory, or "" if it inherits the server's root.
	std::string getRoot() const;
	AutoindexState getAutoindexState() const;
	/// @return The location's index filenames, or empty if it inherits the server's list.
	std::vector<std::string> getIndex() const;
	size_t getClientMaxBodySize() const;
	/// @return True if this location explicitly sets client_max_body_size (vs. inheriting the server's).
	bool hasClientMaxBodySize() const;

	std::string getCgiExt() const;
	std::string getCgiPath() const;
	std::string getUploadStore() const;

	std::vector<std::string> getAllowMethods() const;
	/// @return True if `method` is allowed here — always true when allow_methods was never set for this location.
	bool isMethodAllowed(const std::string& method) const;
};

#endif
