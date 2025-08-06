FROM python:3.10.10-slim
RUN apt-get update -y && apt-get install -y git

RUN pip install --upgrade pip

COPY ./student_alloc /app
COPY ./resources/ProjectAssignment-master /app/solver
COPY ./.env /app/.env
COPY ./gurobi.lic /app/gurobi.lic

COPY ./req.docker.txt .
RUN pip install -r req.docker.txt

RUN mkdir -p /var/log/gunicorn/
RUN mkdir -p /var/run/gunicorn/

WORKDIR /app

COPY ./entrypoint.sh .

RUN chmod +x ./entrypoint.sh
ENTRYPOINT ["sh","./entrypoint.sh"]