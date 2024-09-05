FROM golang:1.21-alpine AS builder

RUN apk update && \
    apk add --no-cache upx

WORKDIR /app
COPY . /app

RUN --mount=type=cache,target=/root/.cache/go-build \
    --mount=type=cache,target=/go/pkg/mod \
        go build \
            -ldflags="-s -w" \
            -o /app/dedcleaner
RUN upx --best --ultra-brute /app/dedcleaner

FROM scratch

COPY --from=builder /app/dedcleaner /dedcleaner

CMD ["/dedcleaner"]
