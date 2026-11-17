#pragma once

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

// Los clientes deben vivir hasta después de http.end().
inline bool beginHttp(HTTPClient& http, WiFiClient& plain,
                      WiFiClientSecure& secure, const String& url) {
    http.setConnectTimeout(4000);
    http.setTimeout(5000);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    if (url.startsWith("https://")) {
        // Compatibilidad con la base recuperada; pendiente provisionar CA.
        secure.setInsecure();
        secure.setHandshakeTimeout(5);
        return http.begin(secure, url);
    }
    return url.startsWith("http://") && http.begin(plain, url);
}
