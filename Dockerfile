FROM alpine AS build-env

RUN apk add --no-cache build-base

WORKDIR /app
COPY . .
RUN make

FROM alpine AS runtime

RUN apk add --no-cache python3 py3-matplotlib

RUN mkdir /output

WORKDIR /app
COPY --from=build-env /app/bus_off_sim.out .
COPY plot.py .

ENTRYPOINT ["./bus_off_sim.out"]
