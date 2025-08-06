FROM nginx:stable

RUN mkdir -p /etc/nginx/ssl/cert/ \
	&& mkdir -p /etc/nginx/ssl/key/ \
	&& mkdir -p /etc/nginx/ssl/dh/ \
	&& openssl req \
		-x509 \
		-subj "/C=DK/ST=Despair/L=South-Park/O=assembly/CN=www.adsigno.sdu.dk" \
		-nodes \
		-days 365 \
		-newkey rsa:2048 \
		-keyout /etc/nginx/ssl/key/nginx-selfsigned.key \
		-out /etc/nginx/ssl/cert/nginx-selfsigned.crt \
	&& openssl dhparam -out /etc/nginx/ssl/dh/dhparam.pem 2048

RUN mkdir -p /etc/nginx/snippets/
COPY ./snippets/self-signed.conf /etc/nginx/snippets/self-signed.conf
COPY ./snippets/ssl-params.conf /etc/nginx/snippets/ssl-params.conf
COPY ./sites-available/adsigno.sdu.dk /etc/nginx/conf.d/default.conf
