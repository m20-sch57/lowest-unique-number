FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y build-essential python3 python3-pip
RUN pip3 install flask flask_socketio
RUN pip3 install numpy scipy torch --extra-index-url https://download.pytorch.org/whl/cpu

COPY . /lowest-unique-number
WORKDIR /lowest-unique-number

RUN make

ENTRYPOINT [ "python3", "serve.py" ]
