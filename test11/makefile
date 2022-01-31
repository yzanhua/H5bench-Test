APP_HOME=/homes/zhd1108/H5BENCH/native

export PATH := ${APP_HOME}:$(PATH)
all:
	rm -rf full-test
	rm -f configuration-*.log
	rm -rf core.*
	${APP_HOME}/h5bench --debug configuration.json
clean:
	rm -rf full-test
	rm -f configuration-*.log
	rm -rf core.*
