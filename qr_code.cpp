// Adapted from https://github.com/nayuki/QR-Code-generator

#define qrcodegen_VERSION_MIN   1  // The minimum version number supported in the QR Code Model 2 standard
#define qrcodegen_VERSION_MAX  11  // The maximum version number supported by our code

#define qrcodegen_BUFFER_LEN_FOR_VERSION(n)  ((((n) * 4 + 17) * ((n) * 4 + 17) + 7) / 8 + 1)

#define qrcodegen_BUFFER_LEN_MAX  qrcodegen_BUFFER_LEN_FOR_VERSION(qrcodegen_VERSION_MAX)
