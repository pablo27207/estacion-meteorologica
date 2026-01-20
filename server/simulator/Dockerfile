# Simulador de estación meteorológica
FROM node:20-alpine

WORKDIR /app

COPY package.json ./
COPY simulate-station.js ./

ENV NODE_ENV=production

CMD ["node", "simulate-station.js"]
