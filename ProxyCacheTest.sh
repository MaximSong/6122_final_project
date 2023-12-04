# Number of requests to test
REQUEST_COUNT=2

# Create a temporary directory to track successful requests
temp_dir=$(mktemp -d)
trap 'rm -rf -- "$temp_dir"' EXIT

# Function to send a request and measure the response time
function send_request {
    local index=$1  # Add an index parameter to the function

    # Record the start time
    start_time=$(date +%s%N)

    # Send a GET request and capture the HTTP status code
    http_code=$(curl -s -o /dev/null -w "%{http_code}" -X GET -x http://localhost:30308 \
             http://baidu.com/)

    # Record the end time
    end_time=$(date +%s%N)

    # Calculate the response time
    response_time=$(( (end_time - start_time) / 1000000 )) # Convert nanoseconds to milliseconds

    # Print the response time
    echo "Request $index: Response Time: ${response_time}ms"

    # Check if the HTTP status code is 200
    if [ "$http_code" -eq 200 ]; then
        # Create a file for each successful response using the index
        touch "$temp_dir/success_$index"
    fi
}

# Sequentially run the request function
for i in $(seq 1 $REQUEST_COUNT); do
    send_request $i
done

# Count the number of successful requests
success_count=$(find "$temp_dir" -type f | wc -l)

# Output the number of successful responses
echo "Number of successful responses: $success_count"
