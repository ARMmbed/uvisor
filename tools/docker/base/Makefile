all: build

IMAGE_TAG:="docker.io/meriac/mbed-base"

build:
	docker build -t $(IMAGE_TAG) .

run:
	docker run -i -t $(IMAGE_TAG)

publish: build
	docker push $(IMAGE_TAG)

clean:
	-docker rm  $(shell docker ps -a -q --filter ancestor=$(IMAGE_TAG))
	-docker rmi $(shell docker images -q $($IMAGE_TAG))
