# Live-browser test boundary

Normal Python HTTPS GET succeeded for installer/projects/samples with TLS verification; exact HTTP 200 byte hashes retained in live-routes.json. Normal Chromium navigation to the installer failed net::ERR_CERT_AUTHORITY_INVALID in this environment. No browser TLS validation was bypassed.

A proposed command to add the environment-provided proxy root to /home/agent/.pki/nssdb was rejected before execution by automatic approval review: it would persistently alter HTTPS trust beyond the website task. The action remains DENIED and was not retried, routed around or delegated. No certificate trust setting changed. Independent local product testing and verified HTTPS source/HTTP checks continue; live-origin browser qualification remains NOT_RUN after the failed navigation. The authorized publisher/next tester must exercise live journeys in a normally trusted browser.
