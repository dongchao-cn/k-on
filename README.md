k-on
====

* `tsumugi` Client，负责发出请求

* `yui` Server，负责处理用户请求

* `azusa` Client与Server间的Proxy，负责维持大量Client的链接，并把请求转发给Server

	./bin/azusa 0.0.0.0 8000 2 127.0.0.1 8001