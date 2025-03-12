#include "stdafx.h"
#include <iostream>
#include <string>

#include "CRestApiClient.h"

CRestApiClient::CRestApiClient(const CString& baseUrl) : m_baseUrl(baseUrl)
{
}

rapidjson::Document CRestApiClient::DoRequest(const CString& endPoint, REQUEST_METHOD method, const rapidjson::Document& data)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	switch (method)
	{
	case METHOD_GET:
		return SendRequest(endPoint, method);
	case METHOD_POST:
	case METHOD_PUT:
	{
		if (data == NULL)
		{
			return SendRequest(endPoint, method);
		}
		data.Accept(writer);
		return SendRequest(endPoint, method, CStringA(buffer.GetString()));
	}
	case METHOD_PATCH:
		if (data == NULL)
		{
			return SendPatchRequest(endPoint);
		}
		data.Accept(writer);
		return SendPatchRequest(endPoint, CStringA(buffer.GetString()));
	default:
		return rapidjson::Document();
	}
}

rapidjson::Document CRestApiClient::SendRequest(const CString& endPoint, REQUEST_METHOD method, const CStringA& data)
{
	CInternetSession session(_T("Rest client"));
	CHttpConnection* pConnection = nullptr;
	CHttpFile* pFile = nullptr;
	CString result;
	rapidjson::Document response;

	try {

		CString strServerName;
		CString strObject;
		INTERNET_PORT nPort;
		DWORD dwServiceType;
		CString encodedUrl = m_baseUrl + endPoint;
		//encodedUrl.Replace(_T("&"), _T("%26"));

		if (!AfxParseURL(encodedUrl, dwServiceType, strServerName, strObject, nPort))
		{
			m_lastError = "Ошибка парсинга url";
			return FALSE;
		}

		CString logMsg;
		logMsg.Format(_T("URL: %s, Server: %s, Port: %d, Object: %s"), encodedUrl, strServerName, nPort, strObject);

		pConnection = session.GetHttpConnection(strServerName, nPort);

		pFile = pConnection->OpenRequest(method, strObject);

		CString headers = _T("Content-Type: application/json; charset=UTF-8\r\n")
			_T("Cache-Control: no-cache, no-store, must-revalidate\r\n")
			_T("Pragma: no-cache\r\n")
			_T("Expires: 0\r\n");
		if (!pFile->AddRequestHeaders(headers))
		{
			m_lastError = "Ошибка Добавления заголовка";
			return FALSE;
		}

		if (!data.IsEmpty() && (method == METHOD_POST || method == METHOD_PUT)) {
			CStringA utf8Data(data, CP_UTF8);
			if (!pFile->SendRequest(NULL, 0, (LPVOID)utf8Data.GetString(), utf8Data.GetLength()))
			{
				m_lastError = "Ошибка отправки данных";
			}
		}
		else {
			if (!pFile->SendRequest())
			{
				m_lastError = "Ошибка отправки данных";
			}
		}

		DWORD dwStatusCode;
		pFile->QueryInfoStatusCode(dwStatusCode);

		if (dwStatusCode == HTTP_STATUS_OK || dwStatusCode == HTTP_STATUS_CREATED) {
			char buffer[1024];
			UINT bytesRead;
			while ((bytesRead = pFile->Read(buffer, sizeof(buffer) - 1)) > 0) {
				buffer[bytesRead] = '\0';
				result += buffer;
			}

			response.Parse((LPCTSTR)result);
		}
		else {
			CString response;
			char buffer[1024];
			UINT bytesRead;
			while ((bytesRead = pFile->Read(buffer, sizeof(buffer) - 1)) > 0) {
				buffer[bytesRead] = 0;
				response += buffer;
			}
			m_lastError = "HTTP Error: " + response;
		}
	}
	catch (CInternetException* pEx)
	{
		TCHAR szError[1024];
		pEx->GetErrorMessage(szError, 1024);
		m_lastError = "Internet Exception: " + CString(szError);
		pEx->Delete();
	}

	if (pFile) pFile->Close();
	if (pConnection) pConnection->Close();
	session.Close();

	return response;
}

rapidjson::Document CRestApiClient::SendPatchRequest(const CString& endPoint, const CStringA& data)
{
	CString result;
	rapidjson::Document response;

	HINTERNET hInternet = InternetOpen(
		_T("Rest client"),
		INTERNET_OPEN_TYPE_DIRECT,
		NULL,
		NULL,
		0
	);
	if (!hInternet) {
		m_lastError = "Ошибка открытия коннекта";
		return FALSE;
	}

	CString url, strPort;
	if (m_baseUrl.Find(":") != -1)
	{
		int pos = 0;
		url = m_baseUrl.Tokenize(":", pos);
		strPort = m_baseUrl.Tokenize(":", pos);
	}
	int port = _ttoi(strPort);

	HINTERNET hConnect = InternetConnect(
		hInternet,
		_T(m_baseUrl), 
		port,//INTERNET_DEFAULT_HTTPS_PORT,
		NULL,
		NULL,
		INTERNET_SERVICE_HTTP,
		0,
		0
	);
	if (!hConnect) {
		m_lastError = "Ошибка подключения к серверу.";
		InternetCloseHandle(hInternet);
		return FALSE;
	}

	HINTERNET hRequest = HttpOpenRequest(
		hConnect,
		"PATCH", 
		endPoint,
		NULL,
		NULL,
		NULL,
		INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 
		0
	);
	if (!hRequest) {
		m_lastError = "Ошибка открытия HTTP запроса.";
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return FALSE;
	}

	CString headers = _T("Content-Type: application/json; charset=UTF-8\r\n")
		_T("Cache-Control: no-cache, no-store, must-revalidate\r\n")
		_T("Pragma: no-cache\r\n")
		_T("Expires: 0\r\n");
	std::wstring requestBody = CStringW(data);

	if (!HttpSendRequest(
		hRequest,
		headers,
		wcslen(CStringW(headers)),
		(LPVOID)requestBody.c_str(),
		requestBody.size() * sizeof(wchar_t)
	)) {
		m_lastError = "Ошибка отправки HTTP запроса.";
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return FALSE;
	}

	char buffer[1024];
	DWORD bytesRead;
	while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
		buffer[bytesRead] = '\0';
		result += buffer;
	}

	response.Parse((LPCTSTR)result);

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	return response;
}
