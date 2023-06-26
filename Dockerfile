FROM ubuntu:latest

WORKDIR /code

RUN apt-get update && apt-get install -y gcc
RUN echo "alias exit='rm -rf /code/*.gch && exit'" >> ~/.bashrc

CMD ["tail", "-f", "/dev/null"]

# docker run --name str str
# docker run -it -v /Users/izaias/dev/Trabalho/Code/s
