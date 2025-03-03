#pragma once

#include <afxinet.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

enum method {
	METHOD_GET = CHttpConnection::HTTP_VERB_GET,
	METHOD_POST = CHttpConnection::HTTP_VERB_POST,
	METHOD_PUT = CHttpConnection::HTTP_VERB_PUT
} typedef REQUEST_METHOD;

class CRestApiClient
{
public:
	CRestApiClient(const CString& baseUrl);

	rapidjson::Document DoRequest(const CString& endPoint, REQUEST_METHOD method, const rapidjson::Document& data = nullptr);

	CString GetLastError() { return m_lastError; };

private:
	CString m_baseUrl;

	CString m_lastError;

	rapidjson::Document SendRequest(const CString& endPoint, const short method, const CString& data = "");
};

