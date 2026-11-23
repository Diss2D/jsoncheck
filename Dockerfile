FROM ubuntu:24.04

ARG APP_VERSION

COPY dist/jsoncheck_${APP_VERSION}_amd64.deb /tmp/jsoncheck.deb

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       /tmp/jsoncheck.deb curl \
    && rm -rf /var/lib/apt/lists/* /tmp/jsoncheck.deb

ENV APP_PORT=8080

USER 10001

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s \
    CMD curl -fsS http://localhost:8080/health || exit 1

CMD ["jsoncheck", "serve"]
