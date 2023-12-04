# Set the concurrency level
CONCURRENCY=5

# Create a temporary directory for tracking successes
temp_dir=$(mktemp -d)
trap 'rm -rf -- "$temp_dir"' EXIT

# Function to send a request and print the full response
function send_request {
    local index=$1  # Add an index parameter to the function

    # Send a GET request and capture the full response including headers
    full_response=$(curl -s -i -X GET -x http://localhost:30308 \
             http://baidu.com/)

    # Extract the HTTP status code
    http_code=$(echo "$full_response" | grep -o -E "^HTTP/1\.[01] [0-9]+" | awk '{print $2}')
    
    # Print the full response
    echo "$full_response"
    echo "----------"

    # Check if the HTTP status code is 200
    if [ "$http_code" -eq 200 ]; then
        # Create a file for each successful response using the index
        touch "$temp_dir/success_$index"
    fi
}

# Run the function concurrently
for i in $(seq 1 $CONCURRENCY); do
    send_request $i &  # Pass the index to the function
done

# Wait for all background processes to complete
wait

# Count the number of success files
success_count=$(find "$temp_dir" -type f | wc -l)

# Output the number of successful responses
echo "Number of successful responses: $success_count"