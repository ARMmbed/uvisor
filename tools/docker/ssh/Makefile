all: build

PORT:=30303
IMAGE_PREFIX=docker.io/meriac
IMAGE_NAME=mbed-ssh
IMAGE_TAG=$(IMAGE_PREFIX)/$(IMAGE_NAME)

build: $(ENTRYPOINT)
	docker build -t $(IMAGE_TAG) .

run:
	-docker kill $(IMAGE_NAME)
	-docker rm $(IMAGE_NAME)
	docker run --name $(IMAGE_NAME) -d -p $(PORT):22 $(IMAGE_TAG)
	docker logs $(IMAGE_NAME)

ssh:
	ssh -p $(PORT) -o UserKnownHostsFile=/dev/null mbed@localhost

publish: build
	docker push $(IMAGE_TAG)

clean:
	-docker rm $(IMAGE_TAG)
	-docker rm  $(shell docker ps -a -q --filter ancestor=$(IMAGE_TAG))
	-docker rmi $(shell docker images -q $($IMAGE_TAG))
