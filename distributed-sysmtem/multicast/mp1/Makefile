GOCMD=go
GOBUILD=$(GOCMD) build
GOCLEAN=$(GOCMD) clean
GOTEST=$(GOCMD) test
GOGET=$(GOCMD) get
NODE=mp1

all: server

server:
	 $(GOBUILD) -o $(NODE) node.go

clean:
	 go clean
	 rm -f $(NODE)