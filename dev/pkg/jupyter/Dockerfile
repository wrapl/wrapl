FROM wrapl/wrapl:dev

RUN apt-get update && apt-get install -y \
	python3-pip

RUN apt-get clean

RUN pip3 install notebook jupyterthemes

RUN jt -t grade3 -f firacode -fs 10 -nfs 10 -tfs 10 -ofs 10 -cellw 90% -T -N

COPY dev/src/lib/Util/Jupyter/kernel.json /usr/share/jupyter/kernels/wrapl/kernel.json
COPY dev/src/lib/Util/Jupyter/kernel.js /usr/share/jupyter/kernels/wrapl/kernel.js

EXPOSE 9090

WORKDIR /home

ENV LD_LIBRARY_PATH="${PATH}:/usr/lib/riva/Util/eclipse/lib/i386_linux"

CMD jupyter-notebook --port=9090 --ip="0.0.0.0" --allow-root

