FROM pspdev/pspdev:latest

WORKDIR /src
COPY . .

CMD ["sh", "-c", "make clean && make"]
