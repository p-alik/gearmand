=============================
Having Gearman work with SSL.
=============================


If you are not paying for a certificate authority to generate a certificate for you, you will first need to generated a CA for gearmand:

   openssl req -config /etc/pki/tls/openssl.cnf -new -x509 -keyout gearmand-ca-key.pem -out gearmand-ca.pem -days 3650 
   echo "00" > gearmand.srl

You will need to place your ca certificate into the directory you want the server to read it from.

Generate a server certificate for the server to use:

   openssl genrsa -out gearmand.key 1024 
   openssl req -key gearmand.key -new -out gearmand.req
   openssl x509 -req -in gearmand.req -CA gearmand-ca.pem -CAkey gearmand-ca-key.pem -CAserial gearmand.srl -out gearmand.pem 

Generate a client certificate for client/workers to use:

  openssl genrsa -out gearman.key 1024 
  openssl req -key gearman.key -new -out gearman.req 
  openssl x509 -req -in client.req -CA CA.pem -CAkey privkey.pem -CAserial file.srl -out client.pem 
