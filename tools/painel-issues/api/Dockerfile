FROM python:3-alpine
WORKDIR /app
COPY servidor.py /app/servidor.py
ENV PYTHONUNBUFFERED=1
USER 0
EXPOSE 8000
CMD ["python", "/app/servidor.py"]
