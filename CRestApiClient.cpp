#include "stdafx.h"
#include <iostream>
#include <string>

#include "CRestApiClient.h"

CRestApiClient::CRestApiClient(const CString& baseUrl) : m_baseUrl(baseUrl)
{
}

rapidjson::Document CRestApiClient::DoRequest(const CString& endPoint, REQUEST_METHOD method, const rapidjson::Document& data)
{
	switch (method)
	{
	case METHOD_GET:
		return SendRequest(endPoint, method);
	case METHOD_POST:
	case METHOD_PUT:
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		data.Accept(writer);
		return SendRequest(endPoint, method, buffer.GetString());
	}
	default:
		return rapidjson::Document();
	}
}

rapidjson::Document CRestApiClient::SendRequest(const CString& endPoint, const short method, const CString& data)
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

		if (!AfxParseURL(m_baseUrl+endPoint, dwServiceType, strServerName, strObject, nPort))
		{
			return FALSE;
		}

		pConnection = session.GetHttpConnection(strServerName, nPort);

		pFile = pConnection->OpenRequest(method, strObject);

		CString headers = _T("Content-Type: application/json\r\n");
		pFile->AddRequestHeaders(headers);

		if (method == METHOD_POST || method == METHOD_PUT) {
			pFile->SendRequest(NULL, 0, (LPVOID)(LPCTSTR)data, data.GetLength());
		}
		else {
			pFile->SendRequest();
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
			m_lastError = "HTTP Error: " + dwStatusCode;
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
