/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/FileStream.h>
#include <LibProtocol/Client.h>
#include <LibProtocol/Download.h>

namespace Protocol {

Client::Client()
    : IPC::ServerConnection<ProtocolClientEndpoint, ProtocolServerEndpoint>(*this, "/tmp/portal/protocol")
{
    handshake();
}

void Client::handshake()
{
    send_sync<Messages::ProtocolServer::Greet>();
}

bool Client::is_supported_protocol(const String& protocol)
{
    return send_sync<Messages::ProtocolServer::IsSupportedProtocol>(protocol)->supported();
}

template<typename RequestHashMapTraits>
RefPtr<Download> Client::start_download(const String& method, const String& url, const HashMap<String, String, RequestHashMapTraits>& request_headers, ReadonlyBytes request_body)
{
    IPC::Dictionary header_dictionary;
    for (auto& it : request_headers)
        header_dictionary.add(it.key, it.value);

    auto response = send_sync<Messages::ProtocolServer::StartDownload>(method, url, header_dictionary, ByteBuffer::copy(request_body));
    auto download_id = response->download_id();
    if (download_id < 0 || !response->response_fd().has_value())
        return nullptr;
    auto response_fd = response->response_fd().value().take_fd();
    auto download = Download::create_from_id({}, *this, download_id);
    download->set_download_fd({}, response_fd);
    m_downloads.set(download_id, download);
    return download;
}

bool Client::stop_download(Badge<Download>, Download& download)
{
    if (!m_downloads.contains(download.id()))
        return false;
    return send_sync<Messages::ProtocolServer::StopDownload>(download.id())->success();
}

bool Client::set_certificate(Badge<Download>, Download& download, String certificate, String key)
{
    if (!m_downloads.contains(download.id()))
        return false;
    return send_sync<Messages::ProtocolServer::SetCertificate>(download.id(), move(certificate), move(key))->success();
}

void Client::handle(const Messages::ProtocolClient::DownloadFinished& message)
{
    RefPtr<Download> download;
    if ((download = m_downloads.get(message.download_id()).value_or(nullptr))) {
        download->did_finish({}, message.success(), message.total_size());
    }
    m_downloads.remove(message.download_id());
}

void Client::handle(const Messages::ProtocolClient::DownloadProgress& message)
{
    if (auto download = const_cast<Download*>(m_downloads.get(message.download_id()).value_or(nullptr))) {
        download->did_progress({}, message.total_size(), message.downloaded_size());
    }
}

void Client::handle(const Messages::ProtocolClient::HeadersBecameAvailable& message)
{
    if (auto download = const_cast<Download*>(m_downloads.get(message.download_id()).value_or(nullptr))) {
        HashMap<String, String, CaseInsensitiveStringTraits> headers;
        message.response_headers().for_each_entry([&](auto& name, auto& value) { headers.set(name, value); });
        download->did_receive_headers({}, headers, message.status_code());
    }
}

OwnPtr<Messages::ProtocolClient::CertificateRequestedResponse> Client::handle(const Messages::ProtocolClient::CertificateRequested& message)
{
    if (auto download = const_cast<Download*>(m_downloads.get(message.download_id()).value_or(nullptr))) {
        download->did_request_certificates({});
    }

    return make<Messages::ProtocolClient::CertificateRequestedResponse>();
}

}

template RefPtr<Protocol::Download> Protocol::Client::start_download(const String& method, const String& url, const HashMap<String, String>& request_headers, ReadonlyBytes request_body);
template RefPtr<Protocol::Download> Protocol::Client::start_download(const String& method, const String& url, const HashMap<String, String, CaseInsensitiveStringTraits>& request_headers, ReadonlyBytes request_body);
