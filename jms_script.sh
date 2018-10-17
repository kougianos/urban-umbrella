PATHP=$(pwd)
COMMAND="list"
while [[ $# -gt 1 ]]
do
key="$1"
case $key in
    -l|--long-path)
    PATHP="$2"
    shift # past argument
    ;;
    -c|--command)
    COMMAND="$2"
    shift # past argument
    ;;
    -n)
    NI="$2"
    ;;
    *)
            # unknown option
    ;;
esac
shift # past argument or value
done
echo "----------------------------------------------------"
echo "PATH GIVEN  = ${PATHP}"
echo "COMMAND GIVEN = ${COMMAND}"
echo "----------------------------------------------------"
if [ ${COMMAND} == "list" ]; then
	ls -ld ${PATHP}/*/
elif [ ${COMMAND} == "size" ]; then
	if [ -x ${NI+x} ]; then
		du -sk ${PATHP}/*/ | sort -n
	else
		du -sk ${PATHP}/*/ | sort -n | head -"${NI}"
	fi
elif [ ${COMMAND} == "purge" ]; then
	rm -rf ${PATHP}/sdi*
fi
# echo "Number files in SEARCH PATH with EXTENSION:" $(ls -1 "${SEARCHPATH}"/*."${EXTENSION}" | wc -l)
# if [[ -n $1 ]]; then
#     echo "Last line of file specified as non-opt/last argument:"
#     tail -1 $1
# fi