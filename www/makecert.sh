openssl req -newkey rsa:2048 -nodes -keyout main/certs/espkey.pem \
    -x509 -days 36500 -out main/certs/espcert.pem \
    -subj "/CN=ESP32 HTTPS server certificate" \
    -addext "keyUsage=critical,digitalSignature,keyCertSign"
